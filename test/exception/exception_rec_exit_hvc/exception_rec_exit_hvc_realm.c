/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "exception_common_realm.h"

extern sea_parms_ts gSea_parms;

void exception_rec_exit_hvc_realm(void)
{
    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* setting up the exception handler for synchronized exceptions*/
    val_exception_setup(NULL, synchronized_exception_handler);

    val_memset(&gSea_parms, 0, sizeof(gSea_parms));
    /* setup the abort type for the handler */
    gSea_parms.abort_type = EXCEPTION_ABORT_TYPE_HVC;

    /* testing the rec exit dueto hostcall */
    __asm__("hvc #0");

    if (gSea_parms.is_exception_handled && !gSea_parms.status) {
        LOG(TEST, "\tREALM handled the SEA(For HVC) successfully \n", 0, 0);
    }
    else {
         LOG(ERROR, "\tREALM failed to handle the SEA(For HVC) \n", 0, 0);
         val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
         goto test_exit;
    }

test_exit:
    /* reset the abort type */
    gSea_parms.abort_type = EXCEPTION_ABORT_TYPE_NONE;
    val_exception_setup(NULL, NULL);
    val_realm_return_to_host();
}

