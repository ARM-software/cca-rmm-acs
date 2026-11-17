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


static uint64_t da_find_doe_endpoint(val_host_pdev_ts *pdev_obj)
{
    uint32_t num_bdf;
    uint32_t rp_bdf;
    uint32_t status;

    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR)
    {
        LOG(ERROR, "Failed to get PCIe BDF count\n");
        return VAL_ERROR;
    }

    if (val_pcie_find_doe_capability(&num_bdf, &pdev_obj->bdf,
                                     &pdev_obj->doe_cap_base) != VAL_SUCCESS)
    {
        LOG(TEST, "Skipping: No PCIe DOE capability supported device found\n");
        return VAL_SKIP_CHECK;
    }

    status = val_pcie_get_rootport(pdev_obj->bdf, &rp_bdf);
    if (status != PCIE_SUCCESS)
    {
        LOG(ERROR, "Failed to get Root Port for BDF=0x%x\n", pdev_obj->bdf);
        return VAL_ERROR;
    }

    pdev_obj->root_id = (uint16_t)rp_bdf;
    LOG(TEST, "Device BDF=0x%x Root Port BDF=0x%x DOE cap=0x%x\n",
        pdev_obj->bdf, rp_bdf, pdev_obj->doe_cap_base);
    return VAL_SUCCESS;
}

static uint64_t da_alloc_pdev_comm_buffers(val_host_pdev_ts *pdev_obj)
{
    uint64_t req_addr;
    uint64_t resp_addr;

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

    return VAL_SUCCESS;
}

static uint64_t da_create_pdev(val_host_pdev_ts *pdev_obj)
{
    val_host_rmi_pdev_params_ts *pdev_params;
    val_host_pdev_flags_ts pdev_flags;
    uint64_t flags = 0UL;
    uint64_t ret;

    val_memset(pdev_obj, 0, sizeof(*pdev_obj));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

    ret = da_find_doe_endpoint(pdev_obj);
    if (ret != VAL_SUCCESS)
        return ret;

    pdev_flags.spdm = RMI_SPDM_TRUE;
    pdev_flags.category = RMI_PDEV_ENDPOINT_ACCEL_OFF_CHIP;
    val_memcpy(&flags, &pdev_flags, sizeof(pdev_flags));
    val_memcpy(&pdev_obj->pdev_flags, &pdev_flags, sizeof(pdev_flags));

    pdev_obj->pdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev_obj->pdev == 0UL)
    {
        LOG(ERROR, "Failed to allocate PDEV granule\n");
        return VAL_ERROR;
    }

    ret = val_host_rmi_granule_range_delegate(pdev_obj->pdev,
                                              pdev_obj->pdev + PAGE_SIZE);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "Failed to delegate PDEV granule, ret=0x%lx\n", ret);
        return VAL_ERROR;
    }

    pdev_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev_params == NULL)
    {
        LOG(ERROR, "Failed to allocate PDEV params\n");
        return VAL_ERROR;
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

    ret = val_host_rmi_pdev_create(pdev_obj->pdev, (uint64_t)pdev_params);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_PDEV_CREATE failed, ret=0x%lx\n", ret);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t da_pdev_to_ready(val_host_pdev_ts *pdev_obj)
{
    val_host_public_key_params_ts *pubkey_params;
    val_smc_param_ts args;
    uint8_t public_key_algo;
    int rc;

    if (da_alloc_pdev_comm_buffers(pdev_obj) != VAL_SUCCESS)
        return VAL_ERROR;

    if (val_host_dev_communicate(NULL, pdev_obj, NULL, RMI_PDEV_NEEDS_KEY) != VAL_SUCCESS)
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

    if (val_host_dev_communicate(NULL, pdev_obj, NULL, RMI_PDEV_READY) != VAL_SUCCESS)
    {
        LOG(ERROR, "PDEV communicate to READY failed\n");
        return VAL_ERROR;
    }

    args = val_host_rmi_pdev_get_state(pdev_obj->pdev);
    if ((args.x0 != RMI_SUCCESS) || (args.x1 != RMI_PDEV_READY))
    {
        LOG(ERROR, "PDEV did not reach READY, ret=0x%lx state=0x%lx\n",
            args.x0, args.x1);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t da_verify_ready_state(val_host_pdev_ts *pdev_obj)
{
    val_smc_param_ts args;

    args = val_host_rmi_pdev_get_state(pdev_obj->pdev);
    if ((args.x0 != RMI_SUCCESS) || (args.x1 != RMI_PDEV_READY))
    {
        LOG(ERROR, "PDEV is not READY after secure session setup, ret=0x%lx state=0x%lx\n",
            args.x0, args.x1);
        return VAL_ERROR;
    }

    LOG(TEST, "PDEV reached READY after SPDM secure session setup\n");
    return VAL_SUCCESS;
}

static uint64_t da_stop_pdev(val_host_pdev_ts *pdev_obj)
{
    val_smc_param_ts args;

    args = val_host_rmi_pdev_stop(pdev_obj->pdev);
    if (args.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_PDEV_STOP failed, ret=0x%lx\n", args.x0);
        return VAL_ERROR;
    }

    if (val_host_dev_communicate(NULL, pdev_obj, NULL, RMI_PDEV_STOPPED) != VAL_SUCCESS)
    {
        LOG(ERROR, "PDEV stop communication failed\n");
        return VAL_ERROR;
    }

    args = val_host_rmi_pdev_get_state(pdev_obj->pdev);
    if ((args.x0 != RMI_SUCCESS) || (args.x1 != RMI_PDEV_STOPPED))
    {
        LOG(ERROR, "PDEV did not reach STOPPED, ret=0x%lx state=0x%lx\n",
            args.x0, args.x1);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void da_pdev_ready_secure_comm_host(void)
{
    val_host_pdev_ts pdev_obj;
    val_host_rmi_psmmu_params_ts *psmmu_params = NULL;
    val_smc_param_ts args;
    uint32_t pdev_created = 0U;
    uint32_t pdev_stopped = 0U;
    uint32_t psmmu_active = 0U;
    uint32_t psmmu_l2_created = 0U;
    uint32_t final_status = RESULT_PASS(VAL_SUCCESS);
    uint64_t ret;

    if (!val_host_rmm_supports_da())
    {
        LOG(TEST, "Skipping: Device Assignment not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    ret = da_create_pdev(&pdev_obj);
    if (ret == VAL_SKIP_CHECK)
    {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    if (ret != VAL_SUCCESS)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }
    pdev_created = 1U;

    psmmu_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (psmmu_params == NULL)
    {
        LOG(ERROR, "Failed to allocate PSMMU parameters\n");
        final_status = RESULT_FAIL(VAL_ERROR_POINT(2));
        goto cleanup;
    }

    val_memset(psmmu_params, 0, PAGE_SIZE);
    ret = val_host_rmi_psmmu_activate(PLATFORM_OVERRIDE_SMMUV3_BASE,
                                      (uint64_t)psmmu_params);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "PSMMU activation failed, ret=0x%lx\n", ret);
        final_status = RESULT_FAIL(VAL_ERROR_POINT(3));
        goto cleanup;
    }
    psmmu_active = 1U;

    ret = val_host_rmi_psmmu_st_l2_create(PLATFORM_OVERRIDE_SMMUV3_BASE,
                                          pdev_obj.bdf);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "PSMMU L2 stream table create failed, ret=0x%lx\n", ret);
        final_status = RESULT_FAIL(VAL_ERROR_POINT(4));
        goto cleanup;
    }
    psmmu_l2_created = 1U;

    if (da_pdev_to_ready(&pdev_obj) != VAL_SUCCESS)
    {
        final_status = RESULT_FAIL(VAL_ERROR_POINT(5));
        goto cleanup;
    }

    if (da_verify_ready_state(&pdev_obj) != VAL_SUCCESS)
    {
        final_status = RESULT_FAIL(VAL_ERROR_POINT(6));
        goto cleanup;
    }

    if (da_stop_pdev(&pdev_obj) != VAL_SUCCESS)
    {
        final_status = RESULT_FAIL(VAL_ERROR_POINT(7));
        goto cleanup;
    }
    pdev_stopped = 1U;

    args = val_host_rmi_pdev_communicate(pdev_obj.pdev,
                                          (uint64_t)pdev_obj.dev_comm_data);
    if (args.x0 == RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_PDEV_COMMUNICATE succeeded after PDEV_STOPPED\n");
        final_status = RESULT_FAIL(VAL_ERROR_POINT(8));
        goto cleanup;
    }

    LOG(TEST, "PDEV communication rejected after STOPPED, ret=0x%lx\n", args.x0);

cleanup:
    if (psmmu_l2_created != 0U)
    {
        ret = val_host_rmi_psmmu_st_l2_destroy(PLATFORM_OVERRIDE_SMMUV3_BASE,
                                               pdev_obj.bdf);
        if (ret != RMI_SUCCESS)
        {
            LOG(ERROR, "PSMMU L2 stream table destroy failed, ret=0x%lx\n", ret);
            final_status = RESULT_FAIL(VAL_ERROR_POINT(9));
        }
    }

    if (psmmu_active != 0U)
    {
        ret = val_host_rmi_psmmu_deactivate(PLATFORM_OVERRIDE_SMMUV3_BASE);
        if (ret != RMI_SUCCESS)
        {
            LOG(ERROR, "PSMMU deactivate failed, ret=0x%lx\n", ret);
            final_status = RESULT_FAIL(VAL_ERROR_POINT(10));
        }
    }

    if (pdev_created != 0U)
    {
        if (pdev_stopped == 0U)
        {
            if (val_host_pdev_teardown(&pdev_obj, pdev_obj.pdev) != VAL_SUCCESS)
            {
                LOG(ERROR, "PDEV teardown failed\n");
                final_status = RESULT_FAIL(VAL_ERROR_POINT(11));
            }
        }
        else
        {
            ret = val_host_rmi_pdev_destroy(pdev_obj.pdev);
            if (ret != RMI_SUCCESS)
            {
                LOG(ERROR, "RMI_PDEV_DESTROY failed, ret=0x%lx\n", ret);
                final_status = RESULT_FAIL(VAL_ERROR_POINT(12));
            }
        }
    }
    else if (pdev_obj.pdev != 0UL)
    {
        ret = val_host_rmi_granule_range_undelegate(pdev_obj.pdev,
                                                    pdev_obj.pdev + PAGE_SIZE);
        if (ret != RMI_SUCCESS)
            LOG(ERROR, "PDEV granule undelegation failed, ret=0x%lx\n", ret);
    }

    val_set_status(final_status);
}
