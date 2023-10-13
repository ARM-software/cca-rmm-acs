/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"
#include "attestation_realm.h"

void attestation_rem_extend_check_realm(void)
{
    val_smc_param_ts args = {0}, zero_ref = {0};

    val_realm_rsi_measurement_extend(1, 64, 0x05a9bf223fedf80a,
                                            0x9d0da5f73f5c191a,
                                            0x665bf4a0a4a3e608,
                                            0xf2f9e7d5ff23959c,
                                            0, 0, 0, 0);
    /* Read the REM value */
    args = val_realm_rsi_measurement_read(1);
    if (args.x0)
    {
        LOG(ERROR, "\tRSI measurement read failed, ret=%x\n", args.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    /* Compare command output with zero initialized structure, REM values should not zero */
    if (!val_memcmp(&args, &zero_ref, sizeof(args))) {
        LOG(ERROR, "\t Output arguments are zero \n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}
