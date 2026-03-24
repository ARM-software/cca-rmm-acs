/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"
#include "val_realm_device_assignment.h"

#define PAGE_SIZE_ALIGNED __attribute__((aligned(PAGE_SIZE)))

void cmd_rmi_vdev_validate_mapping_realm(void)
{
    val_smc_param_ts cmd_ret;
    val_realm_rsi_host_call_t *gv_realm_host_call;
    val_realm_rsi_dev_mem_flags_ts dev_mem_flags;
    uint64_t vdev_id;
    uint64_t flags;
    uint64_t ipa_base, ipa_top, pa_base;
    uint8_t *shared_report_buff = (val_get_shared_region_base() + TEST_USE_OFFSET4);
    PAGE_SIZE_ALIGNED val_host_pci_tdisp_device_interface_report_struct_t interface_report = {0};
    pci_tdisp_mmio_range_t mmio_range_struct[MAX_MMIO_RANGE_COUNT];
    PAGE_SIZE_ALIGNED val_realm_rsi_vdev_info_ts vdev_info = {0};

    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    vdev_id = gv_realm_host_call->gprs[1];

    cmd_ret = val_realm_rsi_vdev_get_info(vdev_id, (uint64_t)&vdev_info);
    if (cmd_ret.x0)
    {
        LOG(ERROR, "VDEV_GET_INFO failed, ret %x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    if (interface_report_decoding(shared_report_buff, &interface_report, mmio_range_struct))
    {
        LOG(ERROR, "Interface report decoding failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    if (interface_report.mmio_range_count == 0)
    {
        LOG(ERROR, "Interface report has no MMIO ranges\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    val_memset(&dev_mem_flags, 0, sizeof(dev_mem_flags));
    dev_mem_flags.coh = RSI_DEV_MEM_NON_COHERENT;
    dev_mem_flags.order = RSI_DEV_MEM_NOT_LIMITED_ORDER;
    val_memcpy(&flags, &dev_mem_flags, sizeof(dev_mem_flags));

    ipa_base = mmio_range_struct[0].first_page;
    ipa_top = ipa_base + (PAGE_SIZE_4K * mmio_range_struct[0].number_of_pages);
    pa_base = ipa_base;

    do {
        cmd_ret = val_realm_rsi_vdev_validate_mapping(vdev_id,
                                                      ipa_base,
                                                      ipa_top,
                                                      pa_base,
                                                      flags,
                                                      vdev_info.lock_nonce,
                                                      vdev_info.meas_nonce,
                                                      vdev_info.report_nonce);
        if (cmd_ret.x0 != RSI_SUCCESS)
        {
            LOG(ERROR, "RSI_VDEV_VALIDATE_MAPPING failed. %x\n", cmd_ret.x0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }

        if (cmd_ret.x2 != RSI_ACCEPT)
        {
            LOG(ERROR, "Unexpected response. new_base=0x%lx resp=0x%lx\n",
                cmd_ret.x1, cmd_ret.x2);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto exit;
        }

        if ((cmd_ret.x1 == ipa_base) && (cmd_ret.x1 != ipa_top))
        {
            LOG(ERROR, "No progress in validation. new_base=0x%lx\n", cmd_ret.x1);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
            goto exit;
        }

        ipa_base = cmd_ret.x1;
    } while (ipa_base < ipa_top);

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    val_realm_return_to_host();
}
