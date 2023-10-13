/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_exceptions.h"
#include "val_realm_rsi.h"
#include "val_realm_exception.h"
#include "val_realm_framework.h"

/* global instance where testcases can use the exception info*/
sea_params_ts g_sea_params;

bool synchronous_exception_handler(void)
{
    uint64_t esr_el1 = val_esr_el1_read();
    uint64_t far_el1 = val_far_el1_read();
    uint64_t next_pc = val_elr_el1_read() + 4;
    uint64_t ec = esr_el1 >> 26;

    if (g_sea_params.abort_type == EXCEPTION_ABORT_TYPE_HVC)
    {
        /* EC = 0 when Attempted execution of:
         * An HVC instruction when disabled by HCR_EL2.HCD or SCR_EL3.HCE.
         */
        if (ec != EC_UNKNOWN)
        {
            LOG(ERROR, "\tEC class mismatch, expected HVC but received %x\n", ec, 0);
        } else {
            g_sea_params.handler_abort = 1;
        }
    } else if (ec != g_sea_params.abort_type  || far_el1 != g_sea_params.far)
    {
        LOG(ERROR, "\tUnexpected exception detected ec=%x, far=%x\n", ec, far_el1);
    } else
    {
        g_sea_params.handler_abort = 1;
    }

    if (ec == EC_INSTRUCTION_ABORT_SAME_EL)
    {
        val_exception_setup(NULL, NULL);
        val_realm_return_to_host();
    } else {
        /* Skip instruction that triggered the exception. */
        val_elr_el1_write(next_pc);
    }

    /* Indicate that elr_el1 should not be restored. */
    return true;
}

