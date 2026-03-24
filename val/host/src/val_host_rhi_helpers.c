/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_doe.h"
#include "pal_common_support.h"
#include "val_host_pcie.h"
#include "val_timer.h"
#include "val_crypto.h"
#include "val_host_helpers.h"
#include "val_host_da.h"
#include "val_host_rhi.h"

uint32_t g_da_vdev_count;

uint64_t da_init(val_host_realm_ts *realm,
                  val_host_pdev_ts *pdev_obj,
                  val_host_vdev_ts *vdev_obj,
                  uint8_t vdev_state)
{
    uint64_t feature_reg, pdev, ret, req_addr, resp_addr, vdev;
    val_host_pdev_params_ts *pdev_params;
    val_host_vdev_params_ts *vdev_params;
    val_smc_param_ts args;
    uint64_t i, j, flags_pdev, flags_vdev;
    uint32_t num_bdf;
    val_host_vdev_flags_ts vdev_flags;
    int rc;
    uint8_t public_key_algo;
    uint8_t *shared_vca_buff = (val_get_shared_region_base() + TEST_USE_OFFSET1);
    uint8_t *shared_cert_buff = (val_get_shared_region_base() + TEST_USE_OFFSET2);
    uint8_t *shared_pubkey_buff = (val_get_shared_region_base() + TEST_USE_OFFSET3);
    uint64_t vca_size = 0;
    uint64_t cert_size = 0;
    uint64_t pubkey_size = 0;
    val_host_realm_flags_ts realm_flags;
    val_host_public_key_params_ts *pubkey_params;
    val_host_pdev_flags_ts pdev_flags;
    uint64_t aux_count;
    uint32_t rp_bdf, status;

    val_memset(pdev_obj, 0, sizeof(*pdev_obj));
    val_memset(vdev_obj, 0, sizeof(*vdev_obj));
    g_da_vdev_count = 1;

    /* Read Feature Register 0 and check for DA support */
    val_host_rmi_features(0, &feature_reg);
    if (VAL_EXTRACT_BITS(feature_reg, 42, 42) == 0) {
        LOG(ERROR, "DA feature not supported.\n");
        return VAL_ERROR;
    }

    /* Allocate and delegate PDEV */
    pdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!pdev)
    {
        LOG(ERROR, "Failed to allocate memory for pdev");
        return VAL_ERROR;
    } else {
        ret = val_host_rmi_granule_delegate(pdev);
        if (ret)
        {
            LOG(ERROR, "PDEV delegation failed, pdev=0x%x, ret=0x%x", pdev, ret);
            return VAL_ERROR;
        }
    }

    /* Allocate memory for params */
    pdev_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    if (pdev_params == NULL)
    {
        LOG(ERROR, "Failed to allocate memory for pdev_params");
        return VAL_ERROR;
    }
    val_memset(pdev_params, 0, PAGE_SIZE_4K);

    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR)
    {
        return VAL_ERROR;
    }

    if (val_pcie_find_doe_capability(&num_bdf, &pdev_obj->bdf,
                                                     &pdev_obj->doe_cap_base))
    {
        return VAL_ERROR;
    }

    status = val_pcie_get_rootport((uint32_t)pdev_obj->bdf, &rp_bdf);
    if (status != 0)
    {
        return VAL_ERROR;
    }

    LOG(DBG, "\n Device BDF %x and RP %x", pdev_obj->bdf, rp_bdf);

    /* Get device memory regions */
    val_host_get_addr_range(pdev_obj);

    for (i = 0; i < pdev_obj->ncoh_num_addr_range; i++) {
    LOG(DBG, "BAR[%u]: 0x%lx-0x%lx\n", i,
        pdev_obj->ncoh_addr_range[i].base,
        pdev_obj->ncoh_addr_range[i].top - 1);
    }

    /* Populate params */
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));
    pdev_flags.spdm = RMI_SPDM_TRUE;
    pdev_flags.ncoh_ide = RMI_IDE_TRUE;
    val_memcpy(&pdev_params->flags, &pdev_flags, sizeof(pdev_flags));

    /* Get aux granules count */
    args = val_host_rmi_pdev_aux_count(pdev_params->flags);
    if (args.x0)
    {
        return VAL_ERROR;
    }
    aux_count = args.x1;

    pdev_params->hash_algo = RMI_HASH_SHA_512;
    pdev_params->pdev_id = pdev_obj->bdf;
    pdev_params->cert_id = 0;
    pdev_params->root_id = (uint16_t)rp_bdf;
    pdev_params->ecam_addr = PLATFORM_OVERRIDE_PCIE_ECAM_BASE_ADDR_0;
    pdev_params->num_aux = aux_count;

    for (j = 0; j < pdev_params->num_aux; j++)
    {
        pdev_params->aux[j] = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
        if (!pdev_params->aux[j])
        {
            LOG(ERROR, "Failed to allocate memory for aux pdev");
            return VAL_ERROR;
        }
        else
        {
            ret = val_host_rmi_granule_delegate(pdev_params->aux[j]);
            if (ret)
            {
                LOG(ERROR, "pdev delegation failed, pdev=0x%x, ret=0x%x", pdev_params->aux[j],
                                                                               ret);
                return VAL_ERROR;
            }
        }
    }

    pdev_params->ncoh_num_addr_range = pdev_obj->ncoh_num_addr_range;

    for (j = 0; j < pdev_obj->ncoh_num_addr_range; j++) {
        pdev_params->ncoh_addr_range[j] = pdev_obj->ncoh_addr_range[j];
    }

    /* Create pdev */
    args = val_host_rmi_pdev_create(pdev, (uint64_t)pdev_params);
    if (args.x0)
    {
        LOG(ERROR, "PDEV create failed\n");
        return VAL_ERROR;
    }

    args = val_host_rmi_pdev_get_state(pdev);
    if (args.x0)
    {
        LOG(ERROR, "PDEV get state failed\n");
        return VAL_ERROR;
    }

    if (args.x1 != RMI_PDEV_NEW)
    {
        LOG(ERROR, "PDEV state should be PDEV_NEW\n");
        return VAL_ERROR;
    }

    LOG(TEST, "\n\tPDEV Create is successful and PDEV state is PDEV_NEW\n");

    /* Allocate buffer to cache VCA */
    pdev_obj->vca = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_PDEV_VCA_LEN_MAX);
    pdev_obj->vca_len = 0;
    if (pdev_obj->vca == NULL) {
        return VAL_ERROR;
    }

    /* Allocate buffer to cache device certificate */
    pdev_obj->cert_slot_id = 0;
    pdev_obj->cert_chain = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_PDEV_CERT_LEN_MAX);
    pdev_obj->cert_chain_len = 0;
    if (pdev_obj->cert_chain == NULL) {
        return VAL_ERROR;
    }

    /* Allocate buffer to store extracted public key */
    pdev_obj->public_key = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev_obj->public_key == NULL) {
        return VAL_ERROR;
    }
    pdev_obj->public_key_len = PAGE_SIZE;

    /* Allocate buffer to store public key metadata */
    pdev_obj->public_key_metadata = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev_obj->public_key_metadata == NULL) {
        return VAL_ERROR;
    }
    pdev_obj->public_key_metadata_len = PAGE_SIZE;


    /* Allocate memory for req addr buffer */
    req_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!req_addr)
    {
        LOG(ERROR, "Failed to allocate memory for req_addr");
        return VAL_ERROR;
    }

    /* Allocate memory for resp_addr buffer */
    resp_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!resp_addr)
    {
        LOG(ERROR, "Failed to allocate memory for resp_addr");
        return VAL_ERROR;
    }

    pdev_obj->pdev = pdev;
    pdev_obj->dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    pdev_obj->dev_comm_data->enter.req_addr = req_addr;
    pdev_obj->dev_comm_data->enter.resp_addr = resp_addr;

    if (val_host_dev_communicate(NULL, pdev_obj, NULL, RMI_PDEV_NEEDS_KEY))
    {
        LOG(ERROR, "PDEV communicate failed ");
        return VAL_ERROR;
    }

    LOG(TEST, "\n\tPDEV state has been changed to PDEV_NEEDS_KEY\n");

    /* Get public key */
    rc = val_host_get_public_key_from_cert_chain(pdev_obj->cert_chain,
                         pdev_obj->cert_chain_len,
                         pdev_obj->public_key,
                         &pdev_obj->public_key_len,
                         pdev_obj->public_key_metadata,
                         &pdev_obj->public_key_metadata_len,
                         &public_key_algo);
    if (rc != 0) {
        LOG(ERROR, "Get public key failed\n");
        return VAL_ERROR;
    }

    if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P256) {
        pdev_obj->public_key_sig_algo = RMI_SIG_ECDSA_P256;
    } else if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P384) {
        pdev_obj->public_key_sig_algo = RMI_SIG_ECDSA_P384;
    } else {
        pdev_obj->public_key_sig_algo = RMI_SIG_RSASSA_3072;
    }

    pubkey_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!pubkey_params)
    {
        LOG(ERROR, "\tFailed to allocate memory for pub_key\n");
        return VAL_ERROR;
    }

    val_memset(pubkey_params, 0, PAGE_SIZE_4K);
    val_memcpy(pubkey_params->key, pdev_obj->public_key, pdev_obj->public_key_len);
    val_memcpy(pubkey_params->metadata, pdev_obj->public_key_metadata,
                                pdev_obj->public_key_metadata_len);
    pubkey_params->key_len = pdev_obj->public_key_len;
    pubkey_params->metadata_len = pdev_obj->public_key_metadata_len;
    pubkey_params->algo = pdev_obj->public_key_sig_algo;

    args = val_host_rmi_pdev_set_pubkey(pdev_obj->pdev, (uint64_t)pubkey_params);
    if (args.x0)
    {
        LOG(ERROR, "Pdev set pub key failed %lx\n", args.x0);
        return VAL_ERROR;
    }

    args = val_host_rmi_pdev_get_state(pdev_obj->pdev);
    if (args.x0)
    {
        LOG(ERROR, "PDEV get state failed\n");
        return VAL_ERROR;
    }

    /* PDEV state should be RMI_PDEV_HAS_KEY */
    if (args.x1 != RMI_PDEV_HAS_KEY)
    {
        LOG(ERROR, "PDEV state should be PDEV_HAS_KEY\n");
        return VAL_ERROR;
    }

    LOG(TEST, "\n\tPDEV state has been changed to PDEV_HAS_KEY\n");

    if (val_host_dev_communicate(NULL, pdev_obj, NULL, RMI_PDEV_READY))
    {
        LOG(ERROR, "PDEV communicate failed ");
        return VAL_ERROR;
    }

    LOG(TEST, "\n\tPDEV state has been changed to RMI_PDEV_READY\n");

    vca_size = pdev_obj->vca_len;
    val_memcpy(shared_vca_buff, pdev_obj->vca, vca_size);

    cert_size = pdev_obj->cert_chain_len;
    val_memcpy(shared_cert_buff, pdev_obj->cert_chain, cert_size);

    pubkey_size = pdev_obj->public_key_len;
    val_memcpy(shared_pubkey_buff, pdev_obj->public_key, pubkey_size);

    val_memset(realm, 0, sizeof(*realm));
    val_memset(&realm_flags, 0, sizeof(realm_flags));
    /* Overwrite Realm Parameters */
    realm_flags.da = RMI_FEATURE_TRUE;

    val_host_realm_params(realm);
    val_memcpy(&realm->flags, &realm_flags, sizeof(realm->flags));

    /* Populate realm with one REC*/
    if (val_host_realm_setup(realm, 1))
    {
        LOG(ERROR, "\tRealm setup failed\n");
        return VAL_ERROR;
    }

    /* Create VDEV */
    /* Allocate and delegate VDEV */
    vdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!vdev)
    {
        LOG(ERROR, "Failed to allocate memory for vdev");
        return VAL_ERROR;
    } else {
        ret = val_host_rmi_granule_delegate(vdev);
        if (ret)
        {
            LOG(ERROR, "vdev delegation failed, vdev=0x%x, ret=0x%x", vdev, ret);
            return VAL_ERROR;
        }
    }

    /* Allocate memory for vdev params */
    vdev_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (vdev_params == NULL)
    {
        LOG(ERROR, "Failed to allocate memory for vdev_params");
        return VAL_ERROR;
    }

    val_memset(vdev_params, 0, PAGE_SIZE_4K);

    /* Populate params */
    val_memset(&vdev_flags, 0, sizeof(vdev_flags));
    vdev_params->flags = 0;
    vdev_params->vdev_id = 0;
    vdev_params->tdi_id = pdev_obj->bdf;

    val_memcpy(&flags_pdev, &pdev_obj->pdev_flags, sizeof(pdev_obj->pdev_flags));
    val_memcpy(&flags_vdev, &vdev_flags, sizeof(vdev_flags));
    args = val_host_rmi_vdev_aux_count(flags_pdev, flags_vdev);
    if (args.x0)
    {
        LOG(ERROR, "VDEV AUX count ABI failed, ret=%lx", args.x0);
        return VAL_ERROR;
    }

    vdev_params->num_aux = args.x1;

    for (j = 0; j < vdev_params->num_aux; j++)
    {
        vdev_params->aux[j] = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
        if (!vdev_params->aux[j])
        {
            LOG(ERROR, "Failed to allocate memory for aux vdev");
            return VAL_ERROR;
        }
        else
        {
            ret = val_host_rmi_granule_delegate(vdev_params->aux[j]);
            if (ret)
            {
                LOG(ERROR, "VDEV delegation failed, vdev=0x%x, ret=0x%x",
                                                 vdev_params->aux[j], ret);
                return VAL_ERROR;
            }
        }
    }

    /* Create vdev */
    args = val_host_rmi_vdev_create(realm->rd,  pdev_obj->pdev, vdev, (uint64_t)vdev_params);
    if (args.x0)
    {
        LOG(ERROR, "VDEV create failed");
        return VAL_ERROR;
    }

    args = val_host_rmi_vdev_get_state(vdev);
    if (args.x0)
    {
        LOG(ERROR, "VDEV get state failed ret %d", args.x0);
        return VAL_ERROR;
    }

    /* Check VDEV state is VDEV_NEW */
    if (args.x1 != RMI_VDEV_NEW)
    {
        LOG(ERROR, "VDEV state should be VDEV_NEW, ret %d", args.x1);
        return VAL_ERROR;
    }

    LOG(TEST, "\n\tVDEV Create is successful and VDEV state is VDEV_NEW\n");

    vdev_obj->pdev = pdev;
    vdev_obj->vdev = vdev;
    vdev_obj->dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    vdev_obj->dev_comm_data->enter.req_addr = req_addr;
    vdev_obj->dev_comm_data->enter.resp_addr = resp_addr;

    if (vdev_state == RMI_VDEV_NEW)
        return VAL_SUCCESS;

    if (val_host_dev_communicate(realm, pdev_obj, vdev_obj, RMI_VDEV_UNLOCKED))
    {
        LOG(ERROR, "VDEV communicate failed.\n");
        return VAL_ERROR;
    }

    args = val_host_rmi_vdev_get_state(vdev);
    if (args.x0)
    {
        LOG(ERROR, "VDEV get state failed ret %d", args.x0);
        return VAL_ERROR;
    }

    /* Check VDEV state is VDEV_UNLOCKED */
    if (args.x1 != RMI_VDEV_UNLOCKED)
    {
        LOG(ERROR, "VDEV state should be VDEV_UNLOCKED, ret %d", args.x1);
        return VAL_ERROR;
    }

    LOG(TEST, "\n\tVDEV state is RMI_VDEV_UNLOCKED\n");
    if (vdev_state == RMI_VDEV_UNLOCKED)
        return VAL_SUCCESS;

    args = val_host_rmi_vdev_lock(realm->rd, vdev_obj->pdev, vdev_obj->vdev);
    if (args.x0)
    {
        LOG(ERROR, "\tVDEV_LOCK failed, ret=%x\n", args.x0);
        return VAL_ERROR;
    }

    if (val_host_dev_communicate(realm, pdev_obj, vdev_obj, RMI_VDEV_LOCKED))
    {
        LOG(ERROR, "VDEV communicate failed.\n");
        return VAL_ERROR;
    }

    /* Allocate buffer to cache device measurements */
    vdev_obj->meas = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_VDEV_MEAS_LEN_MAX);
    if (vdev_obj->meas == NULL) {
        return VAL_ERROR;
    }

    vdev_obj->meas_len = 0;
    /* Allocate buffer to cache device interface report */
    vdev_obj->ifc_report = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_VDEV_IFC_REPORT_LEN_MAX);
    vdev_obj->ifc_report_len = 0;
    if (vdev_obj->ifc_report == NULL) {
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

uint64_t da_create_vdev(val_host_realm_ts *realm, val_host_pdev_ts *pdev_obj,
                        val_host_vdev_ts *vdev_obj, uint64_t vdev_id,
                        uint8_t vdev_state)
{
    val_host_vdev_params_ts *vdev_params;
    val_host_vdev_flags_ts vdev_flags;
    val_smc_param_ts args;
    uint64_t vdev;
    uint64_t ret, flags_pdev, flags_vdev, aux_count;

    val_memset(vdev_obj, 0, sizeof(*vdev_obj));

    vdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!vdev)
        return VAL_ERROR;

    ret = val_host_rmi_granule_delegate(vdev);
    if (ret)
        return VAL_ERROR;

    vdev_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (vdev_params == NULL)
        return VAL_ERROR;

    val_memset(vdev_params, 0, PAGE_SIZE_4K);

    val_memset(&vdev_flags, 0, sizeof(vdev_flags));

    vdev_params->flags = 0;
    vdev_params->vdev_id = vdev_id;
    vdev_params->tdi_id = pdev_obj->bdf;
    val_memcpy(&flags_pdev, &pdev_obj->pdev_flags, sizeof(pdev_obj->pdev_flags));
    val_memcpy(&flags_vdev, &vdev_flags, sizeof(vdev_flags));
    args = val_host_rmi_vdev_aux_count(flags_pdev, flags_vdev);
    if (args.x0)
        return VAL_ERROR;

    aux_count = args.x1;
    vdev_params->num_aux = aux_count;

    for (ret = 0; ret < vdev_params->num_aux; ret++)
    {
        vdev_params->aux[ret] = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
        if (!vdev_params->aux[ret])
            return VAL_ERROR;

        if (val_host_rmi_granule_delegate(vdev_params->aux[ret]))
            return VAL_ERROR;
    }

    args = val_host_rmi_vdev_create(realm->rd, pdev_obj->pdev, vdev, (uint64_t)vdev_params);
    if (args.x0)
        return VAL_ERROR;

    args = val_host_rmi_vdev_get_state(vdev);
    if (args.x0 || args.x1 != RMI_VDEV_NEW)
        return VAL_ERROR;

    vdev_obj->pdev = pdev_obj->pdev;
    vdev_obj->vdev = vdev;
    vdev_obj->vdev_id = vdev_id;
    vdev_obj->dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!vdev_obj->dev_comm_data)
        return VAL_ERROR;

    vdev_obj->dev_comm_data->enter.req_addr = pdev_obj->dev_comm_data->enter.req_addr;
    vdev_obj->dev_comm_data->enter.resp_addr = pdev_obj->dev_comm_data->enter.resp_addr;

    if (vdev_state == RMI_VDEV_NEW)
        return VAL_SUCCESS;

    if (val_host_dev_communicate(realm, pdev_obj, vdev_obj, RMI_VDEV_UNLOCKED))
        return VAL_ERROR;

    args = val_host_rmi_vdev_get_state(vdev);
    if (args.x0 || args.x1 != RMI_VDEV_UNLOCKED)
        return VAL_ERROR;

    if (vdev_state == RMI_VDEV_UNLOCKED)
        return VAL_SUCCESS;

    args = val_host_rmi_vdev_lock(realm->rd, vdev_obj->pdev, vdev_obj->vdev);
    if (args.x0)
        return VAL_ERROR;

    if (val_host_dev_communicate(realm, pdev_obj, vdev_obj, RMI_VDEV_LOCKED))
        return VAL_ERROR;

    vdev_obj->meas = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_VDEV_MEAS_LEN_MAX);
    if (vdev_obj->meas == NULL)
        return VAL_ERROR;
    vdev_obj->meas_len = 0;

    vdev_obj->ifc_report = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_VDEV_IFC_REPORT_LEN_MAX);
    if (vdev_obj->ifc_report == NULL)
        return VAL_ERROR;
    vdev_obj->ifc_report_len = 0;

    g_da_vdev_count++;
    return VAL_SUCCESS;
}
