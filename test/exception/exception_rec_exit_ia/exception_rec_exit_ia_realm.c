/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "val_realm_exception.h"
#include "exception_common_realm.h"
#include "val_realm_memory.h"

void (*func_ptr)(void);

void exception_rec_exit_ia_realm(void)
{
    val_memory_region_descriptor_ts mem_desc;
    val_realm_rsi_host_call_t *gv_realm_host_call;
    uint64_t faulting_addr;

    /* setting up the exception handler for synchronized exceptions*/
    val_exception_setup(NULL, synchronous_exception_handler);

    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    faulting_addr = gv_realm_host_call->gprs[1];

    mem_desc.virtual_address = faulting_addr;
    mem_desc.physical_address = faulting_addr;
    mem_desc.length = 0x1000;
    mem_desc.attributes = MT_CODE | MT_REALM;
    if (val_realm_pgt_create(&mem_desc))
    {
        LOG(ERROR, "\tVA to PA mapping failed for stage 1\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto test_exit;
    }

    func_ptr = (void *)(faulting_addr);
    func_ptr();

    LOG(ERROR, "\tRec Exit due to IA not triggered\n", 0, 0);
    val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));

test_exit:
    val_exception_setup(NULL, NULL);
    val_realm_return_to_host();
}
