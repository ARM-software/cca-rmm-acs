/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"

void measurement_rim_order_realm(void)
{
    val_smc_param_ts args = {0,};
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t gv_realm_host_call = {0};

    /* Get the RIM value after activating realm */
    args = val_realm_rsi_measurement_read(0);
    if (args.x0)
    {
        LOG(ERROR, "\tRSI measurement read failed, ret=%x\n", args.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    gv_realm_host_call.imm = VAL_SWITCH_TO_HOST;
    gv_realm_host_call.gprs[0] = args.x0;
    gv_realm_host_call.gprs[1] = args.x1;
    gv_realm_host_call.gprs[2] = args.x2;
    gv_realm_host_call.gprs[3] = args.x3;
    gv_realm_host_call.gprs[4] = args.x4;
    gv_realm_host_call.gprs[5] = args.x5;
    gv_realm_host_call.gprs[6] = args.x6;
    gv_realm_host_call.gprs[7] = args.x7;

    val_realm_rsi_host_call_struct((uint64_t)&gv_realm_host_call);

exit:
    val_realm_return_to_host();
}
