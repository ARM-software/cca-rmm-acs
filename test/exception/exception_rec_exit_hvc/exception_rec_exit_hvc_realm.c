/*
 * Copyright (c) 2023, 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "val_realm_exception.h"
#include "exception_common_realm.h"

extern sea_params_ts g_sea_params;

void exception_rec_exit_hvc_realm(void)
{
    /* setting up the exception handler for synchronized exceptions*/
    val_exception_setup(NULL, synchronous_exception_handler);

    val_memset(&g_sea_params, 0, sizeof(g_sea_params));
    /* setup the abort type for the handler */
    g_sea_params.abort_type = EXCEPTION_ABORT_TYPE_HVC;

    /* HVC instruction execution in AArch64 state - Unkonwn exception taken to Realm */
    __asm__("hvc #0");

    if (g_sea_params.handler_abort) {
        LOG(TEST, "REALM handled the SEA(For HVC) successfully \n");
    }
    else {
         LOG(ERROR, "REALM failed to handle the SEA(For HVC) \n");
         val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
         goto test_exit;
    }

test_exit:
    /* reset the abort type */
    g_sea_params.abort_type = EXCEPTION_ABORT_TYPE_NONE;
    val_exception_setup(NULL, NULL);
    val_realm_return_to_host();
}

