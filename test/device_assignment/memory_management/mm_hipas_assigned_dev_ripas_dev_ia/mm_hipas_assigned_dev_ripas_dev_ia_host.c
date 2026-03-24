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

/*
 * Verify all REC exit fields are zero except those explicitly allowed
 * (VDEV mapping exits, gicv3_*, cnt*, and pmu_ovf_status).
 * Return VAL_ERROR if any unexpected non-zero value is found.
 */
static uint32_t validate_rec_exit_zero_fields(val_host_rec_exit_ts *rec_exit)
{
    val_host_rec_exit_ts exit_copy;
    val_host_rec_exit_ts expected_zero_exit;
    uint8_t *exit_bytes;
    size_t i;
    uint64_t value;

    val_memcpy(&exit_copy, rec_exit, sizeof(exit_copy));
    val_memset(&expected_zero_exit, 0, sizeof(expected_zero_exit));

    exit_copy.exit_reason = 0;
    exit_copy.dev_mem_base = 0;
    exit_copy.dev_mem_top = 0;
    exit_copy.dev_mem_pa = 0;
    exit_copy.vdev_id_1 = 0;

    exit_copy.gicv3_hcr = 0;
    val_memset(exit_copy.gicv3_lrs, 0, sizeof(exit_copy.gicv3_lrs));
    exit_copy.gicv3_misr = 0;
    exit_copy.gicv3_vmcr = 0;

    exit_copy.cntp_ctl = 0;
    exit_copy.cntp_cval = 0;
    exit_copy.cntv_ctl = 0;
    exit_copy.cntv_cval = 0;

    exit_copy.pmu_ovf_status = 0;

    if (!val_memcmp(&exit_copy, &expected_zero_exit, sizeof(exit_copy))) {
        return VAL_SUCCESS;
    }

    exit_bytes = (uint8_t *)&exit_copy;
    for (i = 0; i + sizeof(uint64_t) <= sizeof(exit_copy); i += sizeof(uint64_t)) {
        val_memcpy(&value, exit_bytes + i, sizeof(uint64_t));
        if (value) {
            LOG(ERROR, "\tUnexpected REC exit word offset 0x%lx\n",
                (unsigned long)i);
            return VAL_ERROR;
        }
    }

    return VAL_ERROR;
}

void mm_hipas_assigned_dev_ripas_dev_ia_host(void)
{
    uint64_t feature_reg, pdev, ret, req_addr, resp_addr, vdev;
    val_host_pdev_params_ts *pdev_params;
    val_host_vdev_params_ts *vdev_params;
    val_smc_param_ts args;
    uint64_t i, j;
    uint32_t num_bdf;
    int rc;
    uint8_t public_key_algo;
    uint8_t *shared_vca_buff = (val_get_shared_region_base() + TEST_USE_OFFSET1);
    uint8_t *shared_cert_buff = (val_get_shared_region_base() + TEST_USE_OFFSET2);
    uint8_t *shared_pubkey_buff = (val_get_shared_region_base() + TEST_USE_OFFSET3);
    uint8_t *shared_report_buff = (val_get_shared_region_base() + TEST_USE_OFFSET4);
    uint8_t *shared_measurement_buff = (val_get_shared_region_base() + TEST_USE_OFFSET5);
    uint64_t vca_size = 0;
    uint64_t cert_size = 0;
    uint64_t pubkey_size = 0;
    uint64_t report_size = 0;
    uint64_t measurement_size = 0;
    val_host_realm_ts realm;
    val_host_realm_flags_ts realm_flags;
    val_host_rec_exit_ts *rec_exit = NULL;
    val_host_rec_enter_ts *rec_enter = NULL;
    val_host_public_key_params_ts *pubkey_params;
    val_host_pdev_flags_ts pdev_flags;
    val_host_vdev_flags_ts vdev_flags;
    uint64_t aux_count;
    uint64_t flags_pdev, flags_vdev;
    uint32_t rp_bdf, status;
    val_host_pdev_ts pdev_obj;
    val_host_vdev_ts vdev_obj;
    val_host_vdev_measure_params_ts *vdev_measure_params;

    val_memset(&pdev_obj, 0, sizeof(pdev_obj));
    val_memset(&vdev_obj, 0, sizeof(vdev_obj));

    /* Read Feature Register 0 and check for DA support */
    val_host_rmi_features(0, &feature_reg);
    if (VAL_EXTRACT_BITS(feature_reg, 42, 42) == 0) {
        LOG(ERROR, "DA feature not supported.\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto destroy_realm;
    }

    /* Allocate and delegate PDEV */
    pdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!pdev)
    {
        LOG(ERROR, "Failed to allocate memory for pdev");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    } else {
        ret = val_host_rmi_granule_delegate(pdev);
        if (ret)
        {
            LOG(ERROR, "PDEV delegation failed, pdev=0x%x, ret=0x%x", pdev, ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto destroy_realm;
        }
    }

    /* Allocate memory for params */
    pdev_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    if (pdev_params == NULL)
    {
        LOG(ERROR, "Failed to allocate memory for pdev_params");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }
    val_memset(pdev_params, 0, PAGE_SIZE_4K);

    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    if (val_pcie_find_doe_capability(&num_bdf, &pdev_obj.bdf,
                                                     &pdev_obj.doe_cap_base))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    status = val_pcie_get_rootport((uint32_t)pdev_obj.bdf, &rp_bdf);
    if (status != 0)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    LOG(DBG, "\n Device BDF %x and RP %x", pdev_obj.bdf, rp_bdf);

    /* Get device memory regions */
    val_host_get_addr_range(&pdev_obj);

    for (i = 0; i < pdev_obj.ncoh_num_addr_range; i++) {
    LOG(DBG, "BAR[%u]: 0x%lx-0x%lx\n", i,
        pdev_obj.ncoh_addr_range[i].base,
        pdev_obj.ncoh_addr_range[i].top - 1);
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
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }
    aux_count = args.x1;

    pdev_params->hash_algo = RMI_HASH_SHA_512;
    pdev_params->pdev_id = pdev_obj.bdf;
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
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
            goto destroy_realm;
        }
        else
        {
            ret = val_host_rmi_granule_delegate(pdev_params->aux[j]);
            if (ret)
            {
                LOG(ERROR, "pdev delegation failed, pdev=0x%x, ret=0x%x", pdev_params->aux[j],
                                                                               ret);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
                goto destroy_realm;
            }
        }
    }

    pdev_params->ncoh_num_addr_range = pdev_obj.ncoh_num_addr_range;

    for (j = 0; j < pdev_obj.ncoh_num_addr_range; j++) {
        pdev_params->ncoh_addr_range[j] = pdev_obj.ncoh_addr_range[j];
    }

    /* Create pdev */
    args = val_host_rmi_pdev_create(pdev, (uint64_t)pdev_params);
    if (args.x0)
    {
        LOG(ERROR, "PDEV create failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto destroy_realm;
    }

    args = val_host_rmi_pdev_get_state(pdev);
    if (args.x0)
    {
        LOG(ERROR, "PDEV get state failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_pdev;
    }

    if (args.x1 != RMI_PDEV_NEW)
    {
        LOG(ERROR, "PDEV state should be PDEV_NEW\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto destroy_pdev;
    }

    LOG(TEST, "\n\tPDEV Create is successful and PDEV state is PDEV_NEW\n");

    pdev_obj.pdev = pdev;

    /* Allocate buffer to cache VCA */
    pdev_obj.vca = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_PDEV_VCA_LEN_MAX);
    pdev_obj.vca_len = 0;
    if (pdev_obj.vca == NULL) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto destroy_pdev;
    }

    /* Allocate buffer to cache device certificate */
    pdev_obj.cert_slot_id = 0;
    pdev_obj.cert_chain = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_PDEV_CERT_LEN_MAX);
    pdev_obj.cert_chain_len = 0;
    if (pdev_obj.cert_chain == NULL) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(14)));
        goto destroy_pdev;
    }

    /* Allocate buffer to store extracted public key */
    pdev_obj.public_key = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev_obj.public_key == NULL) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(15)));
        goto destroy_pdev;
    }
    pdev_obj.public_key_len = PAGE_SIZE;

    /* Allocate buffer to store public key metadata */
    pdev_obj.public_key_metadata = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev_obj.public_key_metadata == NULL) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(16)));
        goto destroy_pdev;
    }
    pdev_obj.public_key_metadata_len = PAGE_SIZE;


    /* Allocate memory for req addr buffer */
    req_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!req_addr)
    {
        LOG(ERROR, "Failed to allocate memory for req_addr");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(17)));
        goto destroy_pdev;
    }

    /* Allocate memory for resp_addr buffer */
    resp_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!resp_addr)
    {
        LOG(ERROR, "Failed to allocate memory for resp_addr");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(18)));
        goto destroy_pdev;
    }

    pdev_obj.dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    pdev_obj.dev_comm_data->enter.req_addr = req_addr;
    pdev_obj.dev_comm_data->enter.resp_addr = resp_addr;

    if (val_host_dev_communicate(NULL, &pdev_obj, NULL, RMI_PDEV_NEEDS_KEY))
    {
        LOG(ERROR, "PDEV communicate failed ");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(19)));
        goto destroy_pdev;
    }

    LOG(TEST, "\n\tPDEV state has been changed to PDEV_NEEDS_KEY\n");

    /* Get public key */
    rc = val_host_get_public_key_from_cert_chain(pdev_obj.cert_chain,
                         pdev_obj.cert_chain_len,
                         pdev_obj.public_key,
                         &pdev_obj.public_key_len,
                         pdev_obj.public_key_metadata,
                         &pdev_obj.public_key_metadata_len,
                         &public_key_algo);
    if (rc != 0) {
        LOG(ERROR, "Get public key failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(20)));
        goto destroy_pdev;
    }

    if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P256) {
        pdev_obj.public_key_sig_algo = RMI_SIG_ECDSA_P256;
    } else if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P384) {
        pdev_obj.public_key_sig_algo = RMI_SIG_ECDSA_P384;
    } else {
        pdev_obj.public_key_sig_algo = RMI_SIG_RSASSA_3072;
    }

    pubkey_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!pubkey_params)
    {
        LOG(ERROR, "\tFailed to allocate memory for pub_key\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(21)));
        goto destroy_pdev;
    }

    val_memset(pubkey_params, 0, PAGE_SIZE_4K);
    val_memcpy(pubkey_params->key, pdev_obj.public_key, pdev_obj.public_key_len);
    val_memcpy(pubkey_params->metadata, pdev_obj.public_key_metadata,
                                pdev_obj.public_key_metadata_len);
    pubkey_params->key_len = pdev_obj.public_key_len;
    pubkey_params->metadata_len = pdev_obj.public_key_metadata_len;
    pubkey_params->algo = pdev_obj.public_key_sig_algo;

    args = val_host_rmi_pdev_set_pubkey(pdev_obj.pdev, (uint64_t)pubkey_params);
    if (args.x0)
    {
        LOG(ERROR, "Pdev set pub key failed %lx\n", args.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(22)));
        goto destroy_pdev;
    }

    args = val_host_rmi_pdev_get_state(pdev_obj.pdev);
    if (args.x0)
    {
        LOG(ERROR, "PDEV get state failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(23)));
        goto destroy_pdev;
    }

    /* PDEV state should be RMI_PDEV_HAS_KEY */
    if (args.x1 != RMI_PDEV_HAS_KEY)
    {
        LOG(ERROR, "PDEV state should be PDEV_HAS_KEY\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(24)));
        goto destroy_pdev;
    }

    LOG(TEST, "\n\tPDEV state has been changed to PDEV_HAS_KEY\n");

    if (val_host_dev_communicate(NULL, &pdev_obj, NULL, RMI_PDEV_READY))
    {
        LOG(ERROR, "PDEV communicate failed ");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(25)));
        goto destroy_pdev;
    }

    LOG(TEST, "\n\tPDEV state has been changed to RMI_PDEV_READY\n");

    vca_size = pdev_obj.vca_len;
    val_memcpy(shared_vca_buff, pdev_obj.vca, vca_size);

    cert_size = pdev_obj.cert_chain_len;
    val_memcpy(shared_cert_buff, pdev_obj.cert_chain, cert_size);

    pubkey_size = pdev_obj.public_key_len;
    val_memcpy(shared_pubkey_buff, pdev_obj.public_key, pubkey_size);

    val_memset(&realm, 0, sizeof(realm));
    val_memset(&realm_flags, 0, sizeof(realm_flags));
    /* Overwrite Realm Parameters */
    realm_flags.da = RMI_FEATURE_TRUE;

    val_host_realm_params(&realm);
    val_memcpy(&realm.flags, &realm_flags, sizeof(realm.flags));

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm, 1))
    {
        LOG(ERROR, "\tRealm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(26)));
        goto destroy_pdev;
    }

    /* Create VDEV */
    /* Allocate and delegate VDEV */
    vdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!vdev)
    {
        LOG(ERROR, "Failed to allocate memory for vdev");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(27)));
        goto destroy_pdev;
    } else {
        ret = val_host_rmi_granule_delegate(vdev);
        if (ret)
        {
            LOG(ERROR, "vdev delegation failed, vdev=0x%x, ret=0x%x", vdev, ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(28)));
            goto destroy_pdev;
        }
    }

    /* Allocate memory for vdev params */
    vdev_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (vdev_params == NULL)
    {
        LOG(ERROR, "Failed to allocate memory for vdev_params");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(29)));
        goto destroy_pdev;
    }

    val_memset(vdev_params, 0, PAGE_SIZE_4K);

    /* Populate params */
    val_memset(&vdev_flags, 0, sizeof(vdev_flags));
    vdev_params->flags = 0;
    vdev_params->vdev_id = 0;
    vdev_params->tdi_id = pdev_obj.bdf;

    val_memcpy(&flags_pdev, &pdev_params->flags, sizeof(pdev_params->flags));
    val_memcpy(&flags_vdev, &vdev_flags, sizeof(vdev_flags));

    args = val_host_rmi_vdev_aux_count(flags_pdev, flags_vdev);
    if (args.x0)
    {
        LOG(ERROR, "VDEV AUX count ABI failed, ret value is: %x\n", args.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(30)));
        goto destroy_pdev;
    }

    vdev_params->num_aux = args.x1;

    for (j = 0; j < vdev_params->num_aux; j++)
    {
        vdev_params->aux[j] = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
        if (!vdev_params->aux[j])
        {
            LOG(ERROR, "Failed to allocate memory for aux vdev");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(31)));
            goto destroy_pdev;
        }
        else
        {
            ret = val_host_rmi_granule_delegate(vdev_params->aux[j]);
            if (ret)
            {
                LOG(ERROR, "VDEV delegation failed, vdev=0x%x, ret=0x%x",
                                                 vdev_params->aux[j], ret);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(32)));
                goto destroy_pdev;
            }
        }
    }

    /* Create vdev */
    args = val_host_rmi_vdev_create(realm.rd,  pdev_obj.pdev, vdev, (uint64_t)vdev_params);
    if (args.x0)
    {
        LOG(ERROR, "VDEV create failed");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(33)));
        goto destroy_pdev;
    }

    args = val_host_rmi_vdev_get_state(vdev);
    if (args.x0)
    {
        LOG(ERROR, "VDEV get state failed ret %d", args.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(34)));
        goto destroy_vdev;
    }

    /* Check VDEV state is VDEV_NEW */
    if (args.x1 != RMI_VDEV_NEW)
    {
        LOG(ERROR, "VDEV state should be VDEV_NEW, ret %d", args.x1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(35)));
        goto destroy_vdev;
    }

    LOG(TEST, "\n\tVDEV Create is successful and VDEV state is VDEV_NEW\n");

    vdev_obj.pdev = pdev;
    vdev_obj.vdev = vdev;
    vdev_obj.dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    vdev_obj.dev_comm_data->enter.req_addr = req_addr;
    vdev_obj.dev_comm_data->enter.resp_addr = resp_addr;

    if (val_host_dev_communicate(&realm, &pdev_obj, &vdev_obj, RMI_VDEV_UNLOCKED))
    {
        LOG(ERROR, "VDEV communicate failed.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(36)));
        goto destroy_vdev;
    }

    /* Allocate buffer to cache device measurements */
    vdev_obj.meas = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_VDEV_MEAS_LEN_MAX);
    if (vdev_obj.meas == NULL) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(37)));
        goto destroy_vdev;
    }

    vdev_obj.meas_len = 0;

    /* Allocate buffer to cache device interface report */
    vdev_obj.ifc_report = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_VDEV_IFC_REPORT_LEN_MAX);
    vdev_obj.ifc_report_len = 0;
    if (vdev_obj.ifc_report == NULL) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(38)));
        goto destroy_vdev;
    }

    /* Allocate memory for vdev params */
    vdev_measure_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (vdev_measure_params == NULL)
    {
        LOG(ERROR, "Failed to allocate memory for vdev_measure_params");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(39)));
        goto destroy_vdev;
    }

    val_memset(vdev_measure_params, 0, PAGE_SIZE);

    /*
     * vdev_measure_params->flags is left as 0 as we don't want signed nor
     * raw output.
     */
    /*
     * Set indices value  to (1 << 254) to retrieve all measurements
     * supported by the device.
     */
    vdev_measure_params->indices[31] = (unsigned char)1U << 6U;

    args = val_host_rmi_vdev_get_measurements(realm.rd, vdev_obj.pdev, vdev_obj.vdev,
                        (uint64_t)vdev_measure_params);
    if (args.x0)
    {
        LOG(ERROR, "\tVDEV_GET_MEASUREMENTS failed, ret=%x\n", args.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(40)));
        goto destroy_vdev;
    }

    if (val_host_dev_communicate(&realm, &pdev_obj, &vdev_obj, RMI_VDEV_UNLOCKED))
    {
        LOG(ERROR, "VDEV communicate failed.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(41)));
        goto destroy_vdev;
    }

    args = val_host_rmi_vdev_lock(realm.rd, vdev_obj.pdev, vdev_obj.vdev);
    if (args.x0)
    {
        LOG(ERROR, "\tVDEV_LOCK failed, ret=%x\n", args.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(42)));
        goto destroy_vdev;
    }

    if (val_host_dev_communicate(&realm, &pdev_obj, &vdev_obj, RMI_VDEV_LOCKED))
    {
        LOG(ERROR, "VDEV communicate failed.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(43)));
        goto destroy_vdev;
    }

    args = val_host_rmi_vdev_get_interface_report(realm.rd, vdev_obj.pdev,
                                                           vdev_obj.vdev);
    if (args.x0)
    {
        LOG(ERROR, "VDEV_GET_INTERFACE_REPORT failed, ret=%x\n", args.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(44)));
        goto destroy_vdev;
    }

    if (val_host_dev_communicate(&realm, &pdev_obj, &vdev_obj, RMI_VDEV_LOCKED))
    {
        LOG(ERROR, "VDEV communicate failed.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(45)));
        goto destroy_vdev;
    }

    args = val_host_rmi_vdev_start(realm.rd, vdev_obj.pdev, vdev_obj.vdev);
    if (args.x0)
    {
        LOG(ERROR, "\tVDEV_START failed, ret=%x\n", args.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(46)));
        goto destroy_vdev;
    }

    if (val_host_dev_communicate(&realm, &pdev_obj, &vdev_obj, RMI_VDEV_STARTED))
    {
        LOG(ERROR, "VDEV communicate failed.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(47)));
        goto destroy_vdev;
    }

    rec_enter = &(((val_host_rec_run_ts *)realm.run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(48)));
        goto destroy_vdev;
    }

    /* Check that REC Exit was due to host call*/
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(49)));
        goto destroy_vdev;
    }

    rec_enter->gprs[1] = vdev_params->vdev_id;

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(50)));
        goto destroy_vdev;
    }

    if (rec_exit->exit_reason != RMI_EXIT_VDEV_REQUEST) {
        LOG(ERROR, "\tUnexcpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason,
                                                                        rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(51)));
        goto destroy_vdev;
    }

    /* Complete the VDEV request */
    args = val_host_rmi_vdev_complete(realm.rec[0], vdev_obj.vdev);
    if (args.x0 != RMI_SUCCESS) {
        LOG(ERROR, "Handling VDEV request failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(52)));
    }

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(53)));
        goto destroy_vdev;
    }

    report_size = vdev_obj.ifc_report_len;
    val_memcpy(shared_report_buff, vdev_obj.ifc_report, report_size);

    measurement_size = vdev_obj.meas_len;
    val_memcpy(shared_measurement_buff, vdev_obj.meas, measurement_size);

    rec_enter->gprs[1] = vca_size;
    rec_enter->gprs[2] = cert_size;
    rec_enter->gprs[3] = pubkey_size;
    rec_enter->gprs[4] = report_size;
    rec_enter->gprs[5] = measurement_size;

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(54)));
        goto destroy_vdev;
    }

    val_pcie_mem_enable(pdev_obj.bdf);

    do
    {
        /* Check that REC Exit was due to RIPAS Change request */
        if (rec_exit->exit_reason != RMI_EXIT_VDEV_REQUEST) {
            LOG(ERROR, "\tUnexcpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason,
                                                                         rec_exit->esr);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(55)));
            goto destroy_vdev;
        }

        /* Complete the VDEV request */
        args = val_host_rmi_vdev_complete(realm.rec[0], vdev_obj.vdev);
        if (args.x0 != RMI_SUCCESS) {
            LOG(ERROR, "Handling VDEV request failed\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(56)));
        }

        ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
        if (ret)
        {
            LOG(ERROR, "\tRec enter failed, ret=%x\n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(57)));
            goto destroy_vdev;
        }

        if (rec_exit->exit_reason != RMI_EXIT_VDEV_MAP) {
            LOG(ERROR, "\tUnexcpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason,
                                                                            rec_exit->esr);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(58)));
            goto destroy_vdev;
        }

        if (validate_rec_exit_zero_fields(rec_exit)) {
            LOG(ERROR, "\tREC exit contains unexpected non-zero fields\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(59)));
            goto destroy_vdev;
        }

        LOG(DBG, "\nrec_exit->dev_mem_base %lx rec_exit->dev_mem_top %lx", rec_exit->dev_mem_base,
                                                                           rec_exit->dev_mem_top);
        LOG(DBG, "\nrec_exit->dev_mem_pa %lx\n", rec_exit->dev_mem_pa);

        ret = val_host_dev_mem_map(&realm, vdev, rec_exit->dev_mem_base,
                           rec_exit->dev_mem_top, rec_exit->dev_mem_pa);
        if (ret)
        {
            LOG(ERROR, "\tval_host_dev_mem_map failed \n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(60)));
            goto destroy_vdev;
        }

        args = val_host_rmi_vdev_validate_mapping(realm.rd, realm.rec[0], pdev, vdev,
             rec_exit->dev_mem_base, rec_exit->dev_mem_top);
        if (args.x0)
        {
            LOG(ERROR, "RMI_VDEV_VALIDATE_MAPPING failed with ret = 0x%x\n", args.x0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(61)));
            goto destroy_vdev;
        }

        ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
        if (ret)
        {
            LOG(ERROR, "\tRec enter failed, ret=%x\n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(62)));
            goto destroy_vdev;
        }

    } while (rec_exit->exit_reason == RMI_EXIT_VDEV_REQUEST);

    LOG(TEST, "\nMappings are validated.\n");

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(63)));
        goto destroy_vdev;
    }

    /* Check that REC Exit was due to VDEV_REQUEST */
    if (rec_exit->exit_reason != RMI_EXIT_VDEV_REQUEST) {
        LOG(ERROR, "\tUnexcpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason,
                                                                        rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(64)));
        goto destroy_vdev;
    }

    /* Complete the VDEV request */
    args = val_host_rmi_vdev_complete(realm.rec[0], vdev_obj.vdev);
    if (args.x0 != RMI_SUCCESS) {
        LOG(ERROR, "Handling VDEV request failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(65)));
    }

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(66)));
        goto destroy_vdev;
    }

    /* Check that REC Exit was due to VDEV_REQUEST */
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "\tUnexcpected REC exit, %d \n", rec_exit->exit_reason);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(67)));
        goto destroy_vdev;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));


destroy_vdev:
    if (vdev_obj.vdev != 0U)
    {
        if (val_host_vdev_teardown(&realm, &pdev_obj, &vdev_obj))
        {
            LOG(ERROR, "VDEV teardown failed\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(69)));
            goto destroy_pdev;
        }
    }

destroy_pdev:
    if (pdev_obj.pdev != 0U)
    {
        if (val_host_pdev_teardown(&pdev_obj, pdev_obj.pdev))
        {
            LOG(ERROR, "PDEV teardown failed\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(70)));
            goto destroy_realm;
        }
    }

    /* Free test resources */
destroy_realm:
    return;
}
