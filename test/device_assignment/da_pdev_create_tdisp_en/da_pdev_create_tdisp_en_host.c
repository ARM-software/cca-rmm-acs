/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_crypto.h"
#include "val_host_da.h"
#include "val_host_helpers.h"
#include "val_host_pcie.h"
#include "val_host_rmi.h"
#include "val_host_rmi_sro.h"

#define PCIE_ECAP_NEXT_CAP_OFFSET_SHIFT           20U
#define PCIE_ECAP_NEXT_CAP_OFFSET_MASK            0xFFFU

#define DVSEC_HDR1_VENDOR_ID_MASK                 0xFFFFU
#define DVSEC_HDR1_REVISION_SHIFT                 16U
#define DVSEC_HDR1_REVISION_MASK                  0xFU
#define DVSEC_HDR2_DVSEC_ID_MASK                  0xFFFFU

#define DA_TDISP_DVSEC_NOT_FOUND                  2U

static uint32_t read_rp_rme_da_tdisp_en(uint32_t rp_bdf, uint32_t *tdisp_en)
{
    uint32_t cap_offset = PCIE_ECAP_START;
    uint32_t reg_value;
    uint32_t hdr1;
    uint32_t hdr2;
    uint32_t ctl1;
    uint32_t cap_id;
    uint32_t vendor_id;
    uint32_t revision;
    uint32_t dvsec_id;

    while (cap_offset != 0U)
    {
        if ((cap_offset < PCIE_ECAP_START) || (cap_offset > PCIE_ECAP_END))
            return VAL_ERROR;

        if (val_pcie_read_cfg(rp_bdf, cap_offset, &reg_value) != PCIE_SUCCESS)
            return VAL_ERROR;

        cap_id = reg_value & PCIE_ECAP_CIDR_MASK;
        if (cap_id == ECID_DVSEC)
        {
            if (val_pcie_read_cfg(rp_bdf,
                                  cap_offset + PLATFORM_PCIE_RME_DA_DVSEC_HDR1_OFFSET,
                                  &hdr1) != PCIE_SUCCESS)
                return VAL_ERROR;

            if (val_pcie_read_cfg(rp_bdf,
                                  cap_offset + PLATFORM_PCIE_RME_DA_DVSEC_HDR2_OFFSET,
                                  &hdr2) != PCIE_SUCCESS)
                return VAL_ERROR;

            vendor_id = hdr1 & DVSEC_HDR1_VENDOR_ID_MASK;
            revision = (hdr1 >> DVSEC_HDR1_REVISION_SHIFT) & DVSEC_HDR1_REVISION_MASK;
            dvsec_id = hdr2 & DVSEC_HDR2_DVSEC_ID_MASK;

            if ((vendor_id == PLATFORM_PCIE_RME_DA_DVSEC_VENDOR_ID) &&
                (revision == PLATFORM_PCIE_RME_DA_DVSEC_REVISION) &&
                (dvsec_id == PLATFORM_PCIE_RME_DA_DVSEC_ID))
            {
                if (val_pcie_read_cfg(rp_bdf,
                    cap_offset + PLATFORM_PCIE_RME_DA_DVSEC_CTL1_OFFSET,
                    &ctl1) != PCIE_SUCCESS)
                    return VAL_ERROR;

                *tdisp_en = (ctl1 & PLATFORM_PCIE_RME_DA_DVSEC_TDISP_EN_MASK) ?
                             1U : 0U;
                LOG(TEST, "Root Port RME DA DVSEC offset=0x%x ctl1=0x%x TDISP_EN=%u\n",
                    cap_offset, ctl1, *tdisp_en);
                return VAL_SUCCESS;
            }
        }

        cap_offset = (reg_value >> PCIE_ECAP_NEXT_CAP_OFFSET_SHIFT) &
                     PCIE_ECAP_NEXT_CAP_OFFSET_MASK;
    }

    return DA_TDISP_DVSEC_NOT_FOUND;
}

static uint64_t find_doe_endpoint_with_rme_da_rp(val_host_pdev_ts *pdev_obj,
                                                 uint32_t *rp_bdf)
{
    uint32_t num_bdf;
    uint32_t tdisp_en;
    uint32_t status;
    uint32_t pcie_rp_bdf;

    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR)
    {
        LOG(ERROR, "Failed to get PCIe BDF count\n");
        return VAL_ERROR;
    }

    while (val_pcie_find_doe_capability(&num_bdf, &pdev_obj->bdf,
                                        &pdev_obj->doe_cap_base) == VAL_SUCCESS)
    {
        status = val_pcie_get_rootport(pdev_obj->bdf, &pcie_rp_bdf);
        if (status != PCIE_SUCCESS)
            continue;

        status = read_rp_rme_da_tdisp_en(pcie_rp_bdf, &tdisp_en);
        if (status == VAL_SUCCESS)
        {
            *rp_bdf = pcie_rp_bdf;
            return VAL_SUCCESS;
        }
    }

    return DA_TDISP_DVSEC_NOT_FOUND;
}

static uint64_t pdev_create_prep_sequence(val_host_pdev_ts *pdev_obj,
                                          val_host_rmi_pdev_params_ts **params_out)
{
    val_host_rmi_pdev_params_ts *pdev_params;
    val_host_pdev_flags_ts pdev_flags;
    uint64_t pdev;
    uint64_t flags;
    uint64_t ret;
    uint32_t rp_bdf;
    uint64_t status;

    val_memset(pdev_obj, 0, sizeof(*pdev_obj));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

    status = find_doe_endpoint_with_rme_da_rp(pdev_obj, &rp_bdf);
    if (status == DA_TDISP_DVSEC_NOT_FOUND)
    {
        LOG(TEST, "Skipping: Root Port Arm RME DA DVSEC not exposed\n");
        return DA_TDISP_DVSEC_NOT_FOUND;
    }

    if (status != VAL_SUCCESS)
    {
        LOG(ERROR, "Failed to find endpoint with RME DA Root Port\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    pdev_flags.spdm = RMI_SPDM_TRUE;
    pdev_flags.category = RMI_PDEV_ENDPOINT_ACCEL_OFF_CHIP;
    val_memcpy(&flags, &pdev_flags, sizeof(pdev_flags));

    pdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev == 0UL)
    {
        LOG(ERROR, "Failed to allocate PDEV granule\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    ret = val_host_rmi_granule_range_delegate(pdev, pdev + PAGE_SIZE);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "Failed to delegate PDEV granule, ret=0x%lx\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    pdev_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev_params == NULL)
    {
        LOG(ERROR, "Failed to allocate PDEV params\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    val_memset(pdev_params, 0, PAGE_SIZE);
    pdev_params->flags = flags;
    pdev_params->hb_base = val_pcie_get_ecam_base(pdev_obj->bdf);
    pdev_params->pdev_id = PCIE_CREATE_BDF_PACKED(pdev_obj->bdf);
    pdev_params->routing_id = PCIE_EXTRACT_BDF_SEG(pdev_obj->bdf);
    pdev_params->id_index = 0UL;
    pdev_params->rid_base = PCIE_CREATE_BDF_PACKED(pdev_obj->bdf);
    pdev_params->rid_top = pdev_params->rid_base + 1U;
    pdev_params->hash_algo = RMI_HASH_SHA_512;

    pdev_obj->pdev = pdev;
    pdev_obj->root_id = (uint16_t)rp_bdf;
    val_memcpy(&pdev_obj->pdev_flags, &pdev_flags, sizeof(pdev_flags));
    *params_out = pdev_params;

    LOG(TEST, "Device BDF=0x%x Root Port BDF=0x%x DOE cap=0x%x\n",
        pdev_obj->bdf, rp_bdf, pdev_obj->doe_cap_base);
    return VAL_SUCCESS;
}

static uint32_t pdev_communicate_to_ready(val_host_pdev_ts *pdev_obj)
{
    val_host_public_key_params_ts *pubkey_params;
    val_smc_param_ts args;
    uint64_t req_addr;
    uint64_t resp_addr;
    uint8_t public_key_algo;
    int rc;

    pdev_obj->vca = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_PDEV_VCA_LEN_MAX);
    pdev_obj->cert_chain = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_PDEV_CERT_LEN_MAX);
    pdev_obj->public_key = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    pdev_obj->public_key_metadata = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    pdev_obj->dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    req_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    resp_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    if ((pdev_obj->vca == NULL) || (pdev_obj->cert_chain == NULL) ||
        (pdev_obj->public_key == NULL) || (pdev_obj->public_key_metadata == NULL) ||
        (pdev_obj->dev_comm_data == NULL) || (req_addr == 0UL) || (resp_addr == 0UL))
    {
        LOG(ERROR, "Failed to allocate PDEV communication resources\n");
        return VAL_ERROR;
    }

    pdev_obj->vca_len = 0U;
    pdev_obj->cert_slot_id = 0U;
    pdev_obj->cert_chain_len = 0U;
    pdev_obj->public_key_len = PAGE_SIZE;
    pdev_obj->public_key_metadata_len = PAGE_SIZE;
    val_memset(pdev_obj->dev_comm_data, 0, PAGE_SIZE);
    pdev_obj->dev_comm_data->enter.req_addr = req_addr;
    pdev_obj->dev_comm_data->enter.rsp_addr = resp_addr;

    if (val_host_dev_communicate(NULL, pdev_obj, NULL, RMI_PDEV_NEEDS_KEY))
    {
        LOG(ERROR, "PDEV communicate to NEEDS_KEY failed\n");
        return VAL_ERROR;
    }

    rc = val_host_get_public_key_from_cert_chain(pdev_obj->cert_chain,
                         pdev_obj->cert_chain_len,
                         pdev_obj->public_key,
                         &pdev_obj->public_key_len,
                         pdev_obj->public_key_metadata,
                         &pdev_obj->public_key_metadata_len,
                         &public_key_algo);
    if (rc != 0)
    {
        LOG(ERROR, "Get public key failed\n");
        return VAL_ERROR;
    }

    if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P256)
        pdev_obj->public_key_sig_algo = RMI_SIG_ECDSA_P256;
    else if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P384)
        pdev_obj->public_key_sig_algo = RMI_SIG_ECDSA_P384;
    else
        pdev_obj->public_key_sig_algo = RMI_SIG_RSASSA_3072;

    pubkey_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pubkey_params == NULL)
    {
        LOG(ERROR, "Failed to allocate public key params\n");
        return VAL_ERROR;
    }

    val_memset(pubkey_params, 0, PAGE_SIZE);
    pubkey_params->key_addr = (uint64_t)pdev_obj->public_key;
    pubkey_params->metadata_addr = (uint64_t)pdev_obj->public_key_metadata;
    pubkey_params->key_len = pdev_obj->public_key_len;
    pubkey_params->metadata_len = pdev_obj->public_key_metadata_len;

    args = val_host_rmi_pdev_set_pubkey(pdev_obj->pdev, (uint64_t)pubkey_params);
    if (args.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_PDEV_SET_PUBKEY failed, ret=0x%lx\n", args.x0);
        return VAL_ERROR;
    }

    args = val_host_rmi_pdev_get_state(pdev_obj->pdev);
    if ((args.x0 != RMI_SUCCESS) || (args.x1 != RMI_PDEV_HAS_KEY))
    {
        LOG(ERROR, "PDEV did not reach HAS_KEY, ret=0x%lx state=0x%lx\n",
            args.x0, args.x1);
        return VAL_ERROR;
    }

    if (val_host_dev_communicate(NULL, pdev_obj, NULL, RMI_PDEV_READY))
    {
        LOG(ERROR, "PDEV communicate to READY failed\n");
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void da_pdev_create_tdisp_en_host(void)
{
    val_host_pdev_ts pdev_obj;
    val_host_rmi_pdev_params_ts *pdev_params = NULL;
    val_host_rmi_psmmu_params_ts *psmmu_params = NULL;
    uint32_t tdisp_en_before = 0U;
    uint32_t tdisp_en_after = 0U;
    uint32_t status;
    uint32_t final_status = RESULT_PASS(VAL_SUCCESS);
    uint32_t pdev_created = 0U;
    uint32_t psmmu_active = 0U;
    uint32_t psmmu_l2_created = 0U;
    uint64_t ret;

    if (!val_host_rmm_supports_da())
    {
        LOG(TEST, "Skipping: DA feature not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    ret = pdev_create_prep_sequence(&pdev_obj, &pdev_params);
    if (ret == DA_TDISP_DVSEC_NOT_FOUND)
    {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    if (ret == VAL_TEST_PREP_SEQ_FAILED)
    {
        LOG(ERROR, "Failed to prepare PDEV create inputs\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    psmmu_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (psmmu_params == NULL)
    {
        LOG(ERROR, "Failed to allocate PSMMU parameters\n");
        final_status = RESULT_FAIL(VAL_ERROR_POINT(8));
        goto cleanup_pdev;
    }
    val_memset(psmmu_params, 0, PAGE_SIZE);

    ret = val_host_rmi_psmmu_activate(PLATFORM_OVERRIDE_SMMUV3_BASE,
                                      (uint64_t)psmmu_params);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "PSMMU activation failed, ret=0x%lx\n", ret);
        final_status = RESULT_FAIL(VAL_ERROR_POINT(9));
        goto cleanup_pdev;
    }
    psmmu_active = 1U;

    ret = val_host_rmi_psmmu_st_l2_create(PLATFORM_OVERRIDE_SMMUV3_BASE,
                                          pdev_obj.bdf);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "PSMMU L2 stream table create failed, ret=0x%lx\n", ret);
        final_status = RESULT_FAIL(VAL_ERROR_POINT(10));
        goto teardown_psmmu;
    }
    psmmu_l2_created = 1U;

    status = read_rp_rme_da_tdisp_en(pdev_obj.root_id, &tdisp_en_before);
    if (status == DA_TDISP_DVSEC_NOT_FOUND)
    {
        LOG(TEST, "Skipping: Root Port RME DA DVSEC not found\n");
        final_status = RESULT_SKIP(VAL_SKIP_CHECK);
        goto teardown_psmmu;
    }

    if (status != VAL_SUCCESS)
    {
        LOG(ERROR, "Failed to read Root Port RME DA DVSEC before PDEV create\n");
        final_status = RESULT_FAIL(VAL_ERROR_POINT(2));
        goto teardown_psmmu;
    }

    ret = val_host_rmi_pdev_create(pdev_obj.pdev, (uint64_t)pdev_params);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_PDEV_CREATE failed, ret=0x%lx\n", ret);
        final_status = RESULT_FAIL(VAL_ERROR_POINT(3));
        goto teardown_psmmu;
    }
    pdev_created = 1U;

    status = read_rp_rme_da_tdisp_en(pdev_obj.root_id, &tdisp_en_after);
    if (status != VAL_SUCCESS)
    {
        LOG(ERROR, "Failed to read Root Port RME DA DVSEC after PDEV create\n");
        final_status = RESULT_FAIL(VAL_ERROR_POINT(4));
        goto teardown_pdev;
    }

    LOG(TEST, "TDISP_EN before PDEV_CREATE=%u after PDEV_CREATE=%u\n",
        tdisp_en_before, tdisp_en_after);

    if (tdisp_en_after != 1U)
    {
        LOG(ERROR, "RMI_PDEV_CREATE did not set Root Port TDISP_EN\n");
        final_status = RESULT_FAIL(VAL_ERROR_POINT(5));
    }

    if (pdev_communicate_to_ready(&pdev_obj) != VAL_SUCCESS)
    {
        LOG(ERROR, "PDEV reference communication flow failed\n");
        final_status = RESULT_FAIL(VAL_ERROR_POINT(6));
    }

teardown_pdev:
    if ((pdev_created != 0U) && val_host_pdev_teardown(&pdev_obj, pdev_obj.pdev))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        final_status = RESULT_FAIL(VAL_ERROR_POINT(7));
    }

teardown_psmmu:
    if (psmmu_l2_created != 0U)
    {
        ret = val_host_rmi_psmmu_st_l2_destroy(PLATFORM_OVERRIDE_SMMUV3_BASE,
                                               pdev_obj.bdf);
        if (ret != RMI_SUCCESS)
        {
            LOG(ERROR, "PSMMU L2 stream table destroy failed, ret=0x%lx\n", ret);
            final_status = RESULT_FAIL(VAL_ERROR_POINT(11));
        }
    }

    if (psmmu_active != 0U)
    {
        ret = val_host_rmi_psmmu_deactivate(PLATFORM_OVERRIDE_SMMUV3_BASE);
        if (ret != RMI_SUCCESS)
        {
            LOG(ERROR, "PSMMU deactivate failed, ret=0x%lx\n", ret);
            final_status = RESULT_FAIL(VAL_ERROR_POINT(12));
        }
    }

    val_set_status(final_status);

cleanup_pdev:
    if ((pdev_obj.pdev != 0UL) && (pdev_created == 0U))
    {
        ret = val_host_rmi_granule_range_undelegate(pdev_obj.pdev,
                                                    pdev_obj.pdev + PAGE_SIZE);
        if (ret != RMI_SUCCESS)
            LOG(ERROR, "PDEV granule undelegation failed, ret=0x%lx\n", ret);
    }
}
