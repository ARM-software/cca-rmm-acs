/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "exception_common_realm.h"

extern sea_parms_ts gSea_parms;
#define EXCEPTION_IA_IPA_REALM_ADDRESS 0x501000
void (*func_ptr)(void);

void exception_rec_exit_ia_realm(void)
{
    val_memory_region_descriptor_ts mem_desc;
    val_pgt_descriptor_ts pgt_desc;
    val_realm_rsi_host_call_t *gv_realm_host_call;
    uint64_t faulting_addr;

    val_set_status(RESULT_PASS(VAL_SUCCESS));
    /* setting up the exception handler for synchronized exceptions*/
    val_exception_setup(NULL, synchronized_exception_handler);

    pgt_desc.ttbr = tt_l0_base;
    pgt_desc.stage = PGT_STAGE1;
    pgt_desc.ias = PGT_IAS;
    pgt_desc.oas = PAGT_OAS;
    mem_desc.virtual_address = EXCEPTION_IA_IPA_REALM_ADDRESS;
    mem_desc.physical_address = EXCEPTION_IA_IPA_REALM_ADDRESS;
    mem_desc.length = 0x1000;
    mem_desc.attributes = ATTR_CODE | ATTR_NS;
    if (val_pgt_create(pgt_desc, &mem_desc))
    {
        LOG(ERROR, "\tVA to PA mapping failed for stage 1\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto test_exit;
    }
    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    faulting_addr = gv_realm_host_call->gprs[1];
    func_ptr = (void *)(faulting_addr + 8);
    func_ptr();
    LOG(ERROR, "\tRec Exit due to IA not triggered\n", 0, 0);
    val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));

test_exit:
    val_exception_setup(NULL, NULL);
    val_realm_return_to_host();
}
