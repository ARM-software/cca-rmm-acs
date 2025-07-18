/*
 * Copyright (c) 2023, 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"
#include "val_exceptions.h"
#include "val_realm_exception.h"
#include "val_realm_memory.h"

extern sea_params_ts g_sea_params;

void mm_ha_hd_access_realm(void)
{
    uint64_t ipa_base;
    uint32_t page_desc;
    uint8_t af_bit_set;
    val_memory_region_descriptor_ts mem_desc;
    val_realm_rsi_host_call_t *gv_realm_host_call;

    /* Below code is executed for REC[0] only */
    LOG(DBG, "In realm_create_realm REC[0], mpdir=%x\n", val_read_mpidr());

    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    val_exception_setup(NULL, synchronous_exception_handler);

    ipa_base = gv_realm_host_call->gprs[1];

    mem_desc.virtual_address = ipa_base;
    mem_desc.physical_address = ipa_base;
    mem_desc.length = PAGE_SIZE;
    mem_desc.attributes = MT_AF_CLEAR | MT_RW_DATA | MT_REALM;
    if (val_realm_pgt_create(&mem_desc))
    {
        LOG(ERROR, "VA to PA mapping failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    val_memset(&g_sea_params, 0, sizeof(g_sea_params));
    g_sea_params.abort_type = EC_DATA_ABORT_SAME_EL;
    g_sea_params.far = ipa_base;

    *(volatile uint32_t *)ipa_base = 0x100;

    if (g_sea_params.handler_abort)
    {
        LOG(ERROR, "Data abort triggered, DBM failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    if (*(volatile uint32_t *)ipa_base != 0x100)
    {
        LOG(ERROR, "DBM: write failed, data mismatch\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    val_realm_read_attributes(ipa_base, &page_desc);

    af_bit_set = ((page_desc & MT_AF_CLEAR) >> MT_AF_SHIFT);
    if (af_bit_set != 0x1)
    {
        LOG(ERROR, "AF flag is not set 1\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
    }

exit:
    val_exception_setup(NULL, NULL);
    val_realm_return_to_host();
}
