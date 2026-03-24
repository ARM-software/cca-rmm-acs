/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"
#include "mbedtls/oid.h"
#include "val_realm_device_assignment.h"
#include "pal_smmuv3_test_engine.h"
#include "val_realm_memory.h"
#include "val_exceptions.h"
#include "val_realm_exception.h"

#define PAGE_SIZE_ALIGNED __attribute__((aligned (PAGE_SIZE)))

#define MAX_TDISP_VERSION       0x10
extern sea_params_ts g_sea_params;

static val_realm_rsi_realm_config_ts realm_config __attribute__((aligned(PAGE_SIZE)));

static uint64_t validate_digests(val_realm_rsi_vdev_info_ts *rsi_device_info)
{
    uint8_t digest[64];
    mbedtls_md_type_t md_type;
    uint8_t digest_size = 0;
    uint8_t *shared_vca_buff = (val_get_shared_region_base() + TEST_USE_OFFSET1);
    uint8_t *shared_cert_buff = (val_get_shared_region_base() + TEST_USE_OFFSET2);
    uint8_t *shared_pubkey_buff = (val_get_shared_region_base() + TEST_USE_OFFSET3);
    uint8_t *shared_report_buff = (val_get_shared_region_base() + TEST_USE_OFFSET4);
    uint8_t *shared_measurement_buff = (val_get_shared_region_base() + TEST_USE_OFFSET5);
    uint64_t vca_buff_size, cert_buff_size, pubkey_buff_size;
    uint64_t report_buff_size, measurement_buff_size;
    uint64_t ret, i;
    val_realm_rsi_host_call_t *gv_realm_host_call;
    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    vca_buff_size = gv_realm_host_call->gprs[1];
    cert_buff_size = gv_realm_host_call->gprs[2];
    pubkey_buff_size = gv_realm_host_call->gprs[3];
    report_buff_size = gv_realm_host_call->gprs[4];
    measurement_buff_size = gv_realm_host_call->gprs[5];
    /*Uncomment once pubkey cache is set by TF-RMM */
    (void)pubkey_buff_size;
    (void)shared_pubkey_buff;

    val_realm_verify_digests_ts verify_digests[] = {
        {
            shared_vca_buff,
            vca_buff_size,
            (void *)rsi_device_info->vca_digest
        },
        {
            shared_cert_buff,
            cert_buff_size,
            (void *)rsi_device_info->cert_digest
        },
        {
            shared_report_buff,
            report_buff_size,
            (void *)rsi_device_info->report_digest
        },
        {
            shared_measurement_buff,
            measurement_buff_size,
            (void *)rsi_device_info->meas_digest
        }
    };

    if (rsi_device_info->hash_algo == RSI_HASH_SHA_256)
    {
        md_type = MBEDTLS_MD_SHA256;
        digest_size = 32;
    }
    else if (rsi_device_info->hash_algo == RSI_HASH_SHA_512)
    {
        md_type = MBEDTLS_MD_SHA512;
        digest_size = 64;
    }
    else
    {
        LOG(ERROR, "Invalid hash alogirithm\n");
        return VAL_ERROR;
    }

    const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(md_type);
    if (md_info == NULL)
    {
        LOG(ERROR, "Invalid hash alogirthm type.\n");
        return VAL_ERROR;
    }

    for (i = 0; i < 4 ; i++)
    {
        /* Compute and compare digest */
        ret = (uint64_t)mbedtls_md(md_info, verify_digests[i].digest_report,
                                         verify_digests[i].digest_report_size, digest);
        if (ret)
        {
            LOG(ERROR, "Mbedtls certificate digests calculate failed 0x%x\n", ret);
            return VAL_ERROR;
        }

        if (val_memcmp((void *)verify_digests[i].digests, digest, digest_size))
        {
            LOG(ERROR, "Digests not matching\n");
            return VAL_ERROR;
        }
    }
    return VAL_SUCCESS;
}

void mm_hipas_assigned_dev_ripas_destroyed_da_realm(void)
{
    uint64_t feature_reg;
    val_smc_param_ts args;
    val_realm_rsi_host_call_t *gv_realm_host_call;
    uint64_t vdev_id;
    PAGE_SIZE_ALIGNED val_realm_rsi_vdev_info_ts vdev_info = {0};
    uint64_t ret, i;
    uint8_t *shared_report_buff = (val_get_shared_region_base() + TEST_USE_OFFSET4);
    PAGE_SIZE_ALIGNED val_host_pci_tdisp_device_interface_report_struct_t interface_report = {0};
    pci_tdisp_mmio_range_t mmio_range_struct[MAX_MMIO_RANGE_COUNT];
    uint64_t ipa_base = 0, ipa_top = 0;
    val_memory_region_descriptor_ts mem_desc;
    uint64_t non_ats_plane;
    uint64_t map_size;
    val_realm_rsi_dev_mem_flags_ts dev_mem_flags;
    uint64_t flags;
    void (*fun_ptr)(void);

    /* Read Feature Register 0 and check for DA support */
    val_realm_rsi_features(0, &feature_reg);
    if (VAL_EXTRACT_BITS(feature_reg, 0, 0) == 0) {
        LOG(ERROR, "DA feature not supported.\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto exit;
    }

    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    vdev_id = gv_realm_host_call->gprs[1];

    args = val_realm_rsi_vdev_get_info(vdev_id, (uint64_t)&vdev_info);
    if (args.x0)
    {
        LOG(ERROR, "VDEV_GET_INFO failed, ret %x\n", args.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    LOG(TEST, "RSI_VDEV_GET_INFO:\n");
    LOG(TEST, "\tflags: 0x%lx\n", vdev_info.flags);
    LOG(TEST, "\tcert_id: 0x%lx\n", vdev_info.cert_id);
    LOG(TEST, "\thash_algo: 0x%lx\n", vdev_info.hash_algo);
    LOG(TEST, "\tlock_nonce: 0x%lx\n", vdev_info.lock_nonce);
    LOG(TEST, "\tmeas_nonce: 0x%lx\n", vdev_info.meas_nonce);
    LOG(TEST, "\treport_nonce: 0x%lx\n", vdev_info.report_nonce);
    LOG(TEST, "\ttdisp_version: 0x%lx\n", vdev_info.tdisp_version);
    LOG(TEST, "\tstate: 0x%lx\n", vdev_info.state);

    /* Validate digests for certificate, interface rpeport and measurements */
    ret = validate_digests(&vdev_info);
    if (ret)
    {
        LOG(ERROR, "VDEV digests mismatched\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    LOG(TEST, "\nDigests are validated.\n");

    /* Digests are matched, now decode the interface report */
    if (interface_report_decoding(shared_report_buff, &interface_report,
                                                    mmio_range_struct))
    {
        LOG(ERROR, "Interface report decoding Failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

        /* Populate params */
    val_memset(&dev_mem_flags, 0, sizeof(dev_mem_flags));
    dev_mem_flags.coh = RSI_DEV_MEM_NON_COHERENT;
    dev_mem_flags.order = RSI_DEV_MEM_NOT_LIMITED_ORDER;

    val_memcpy(&flags, &dev_mem_flags, sizeof(dev_mem_flags));

    for (i = 0; i < interface_report.mmio_range_count; i++)
    {
        ipa_base = mmio_range_struct[i].first_page;
        ipa_top = ipa_base + (PAGE_SIZE_4K * mmio_range_struct[i].number_of_pages);
        args = val_realm_rsi_vdev_validate_mapping(vdev_id,
                                                ipa_base,
                                                ipa_top,
                                                ipa_base,
                                                flags,
                                                vdev_info.lock_nonce,
                                                vdev_info.meas_nonce,
                                                vdev_info.report_nonce);

        if ((args.x0 != RSI_SUCCESS) || (args.x2 != RSI_ACCEPT))
        {
            LOG(ERROR, "VDEV validate mapping failed, ret %x\n", args.x0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }

        map_size = PAGE_SIZE_4K * mmio_range_struct[i].number_of_pages;

        mem_desc.virtual_address = mmio_range_struct[i].first_page;
        mem_desc.physical_address = mmio_range_struct[i].first_page;
        mem_desc.length = map_size;
        mem_desc.attributes = MT_DEVICE_RW | MT_REALM;
        if (val_realm_pgt_create(&mem_desc))
        {
            LOG(ERROR, "VA to PA mapping failed for i=%x \n", i);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto exit;
        }

        args = val_realm_rsi_ipa_state_get(ipa_base, ipa_top);
        if ((args.x0 != RSI_SUCCESS) || (args.x2 != RSI_DEV) || (args.x1 != ipa_top)) {
            LOG(ERROR, "RSI_IPA_STATE_GET failed, ret 0x%lx ipa_base=0x%lx ipa_top=0x%lx "
                    "new_ipa_top=0x%lx ripas=%u\n",
                    args.x0, ipa_base, ipa_top, args.x1, args.x2);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
            goto exit;
        }
    }

    LOG(TEST, "\nMappings are validated.\n");

    val_realm_return_to_host();

    ret = val_realm_rsi_realm_config((uint64_t)(&realm_config));
    if (ret != RSI_SUCCESS) {
        LOG(ERROR, "RSI_REALM_CONFIG failed, ret %x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    non_ats_plane = realm_config.ats_plane + 1UL;
    if (non_ats_plane > realm_config.num_aux_planes) {
        non_ats_plane = 0UL;
    }

    args = val_realm_rsi_vdev_dma_enable(vdev_id, 0, non_ats_plane,
                    vdev_info.lock_nonce,
                    vdev_info.meas_nonce,
                    vdev_info.report_nonce);
    if (args.x0 != RSI_SUCCESS) {
        LOG(ERROR, "RSI_VDEV_DMA_ENABLE failed, ret %x\n", args.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    ipa_base = gv_realm_host_call->gprs[1];

    args = val_realm_rsi_ipa_state_get(ipa_base, (ipa_base + PAGE_SIZE));
    if ((args.x0 != RSI_SUCCESS) || (args.x2 != RSI_DESTROYED)
                                || (args.x1 != (ipa_base + PAGE_SIZE))) {
        LOG(ERROR, "RSI_IPA_STATE_GET failed, ret 0x%lx ipa_top=0x%lx ripas=%u\n",
                args.x0, args.x1, args.x2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

    /* Test intent: RIPAS=DESTROYED and HIPAS=ASSIGNED_DEV, instruction fetch
     * => REC exit due to data abort
     */
    LOG(TEST, "RIPAS=DESTROYED and HIPAS=ASSIGNED_DEV, data access =>"
                        "REC exit due to data abort\n");
    val_exception_setup(NULL, synchronous_exception_handler);
    val_memset(&g_sea_params, 0, sizeof(g_sea_params));
    g_sea_params.abort_type = EC_DATA_ABORT_SAME_EL;
    g_sea_params.far = ipa_base;
    *(volatile uint32_t *)ipa_base = 0x100;
    dsbsy();
    if (g_sea_params.handler_abort)
    {
        LOG(ERROR, "Data abort triggered\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
    }

    val_memset(&g_sea_params, 0, sizeof(g_sea_params));
    g_sea_params.abort_type = EC_INSTRUCTION_ABORT_SAME_EL;
    g_sea_params.far = ipa_base;

    fun_ptr = (void *)ipa_base;
    (*fun_ptr)();

    LOG(ERROR, "Instruction abort not triggered to Realm\n");
    val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));

exit:
    val_exception_setup(NULL, NULL);
    val_realm_return_to_host();
}
