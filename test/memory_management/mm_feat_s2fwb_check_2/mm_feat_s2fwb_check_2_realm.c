/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"
#include "val_exceptions.h"

#define NS_HOST_DATA 0xAAAAAAAA
#define REALM_DATA   0xBBBBBBBB

void mm_feat_s2fwb_check_2_realm(void)
{
    val_realm_rsi_host_call_t *gv_realm_host_call;
    uint32_t status = VAL_SUCCESS;
    val_memory_region_descriptor_ts mem_desc;
    val_pgt_descriptor_ts pgt_desc;
    uint64_t ipa_base;
    uint64_t *addr;

    /* Below code is executed for REC[0] only */
    LOG(DBG, "\tIn realm_create_realm REC[0], mpdir=%x\n", val_read_mpidr(), 0);
    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    ipa_base = gv_realm_host_call->gprs[1];

    /* Map R-EL1 memory as Non-Cacheable */
    pgt_desc.ttbr = tt_l0_base;
    pgt_desc.stage = PGT_STAGE1;
    pgt_desc.ias = PGT_IAS;
    pgt_desc.oas = PAGT_OAS;

    mem_desc.virtual_address = ipa_base;
    mem_desc.physical_address = ipa_base;
    mem_desc.length = PAGE_SIZE;
    mem_desc.attributes = ATTR_RW_DATA_NC | ATTR_NS;
    if (val_pgt_create(pgt_desc, &mem_desc))
    {
        LOG(ERROR, "\tVA to PA mapping failed\n", 0, 0);
        status = status ? status : VAL_ERROR_POINT(1);
    }
    addr = (uint64_t *)ipa_base;
    /* Compare the NS host data */
    if (*addr != NS_HOST_DATA)
    {
        LOG(ERROR, "\tNS host data mismatch\n", 0, 0);
        status = status ? status : VAL_ERROR_POINT(2);
    }

    if (status)
    {
        val_set_status(RESULT_FAIL(status));
        goto exit;
    }

    *addr = REALM_DATA;

exit:
    val_realm_return_to_host();
}
