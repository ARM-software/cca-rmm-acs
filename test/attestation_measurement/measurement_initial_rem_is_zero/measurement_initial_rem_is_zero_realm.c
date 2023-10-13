/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"

void measurement_initial_rem_is_zero_realm(void)
{
    val_smc_param_ts args, zero_ref = {0};

    /* Read the inital REM value */
    args = val_realm_rsi_measurement_read(1);
    if (args.x0)
    {
        LOG(ERROR, "\tRSI measurement read failed, ret=%x\n", args.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    /* Compare command output with zero initialized structure */
    if (val_memcmp(&args, &zero_ref, sizeof(args))) {
        LOG(ERROR, "\t Output arguments are not zero \n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}
