/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_realm_framework.h"

void cmd_rsi_features_realm(void)
{
    uint64_t feat_reg;

    /* Request for RSI_FEATURES  */
    val_realm_rsi_features(0, &feat_reg);

    /* In the current version of the interface, this command returns zero
     *  regardless of the index provided */
    if (feat_reg != 0) {
        LOG(ERROR, "\tReceived non zero value\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}


