 /*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "exception_common_realm.h"

/* global instance where testcases can use the exception info*/
sea_parms_ts gSea_parms;

bool synchronized_exception_handler(void)
{
    gSea_parms.esr_el1 = val_esr_el1_read();
    gSea_parms.ec = gSea_parms.esr_el1 >> 26;
    gSea_parms.is_exception_handled = 1;
    gSea_parms.status = VAL_SUCCESS;

    if (gSea_parms.ec == EC_INSTRUCTION_ABORT_SAME_EL)
    {
        gSea_parms.status = VAL_ERROR;
        val_exception_setup(NULL, NULL);
        val_realm_return_to_host();
    } else if (gSea_parms.abort_type == EXCEPTION_ABORT_TYPE_HVC)
    {
        /* EC = 0 when Attempted execution of:
         * An HVC instruction when disabled by HCR_EL2.HCD or SCR_EL3.HCE.
         */
        if (gSea_parms.ec != EC_UNKNOWN)
        {
            LOG(ERROR, "\tEC class mismatch, expected HVC but received %x\n", gSea_parms.ec, 0);
            gSea_parms.status = VAL_ERROR;
        }
    }

    return true;
}
