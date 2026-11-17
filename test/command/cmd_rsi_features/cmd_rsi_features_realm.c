/*
 * Copyright (c) 2023, 2025-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_realm_framework.h"

void cmd_rsi_features_realm(void)
{
    val_realm_rsi_host_call_t *host_call;
    uint64_t expected_mro;
    uint64_t feat_reg;
    uint64_t ret;

    host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    expected_mro = host_call->gprs[1];

    ret = val_realm_rsi_features(0, &feat_reg);

    /* The default test Realm does not enable DA or ATS. */
    if ((ret != RSI_SUCCESS) ||
        (VAL_EXTRACT_BITS(feat_reg, 3, 63) != 0) ||
        (VAL_EXTRACT_BITS(feat_reg, 0, 0) != RSI_FEATURE_FALSE) ||
        (VAL_EXTRACT_BITS(feat_reg, 1, 1) != expected_mro) ||
        (VAL_EXTRACT_BITS(feat_reg, 2, 2) != RSI_FEATURE_FALSE)) {
        LOG(ERROR, "Invalid RSI feature register 0: ret=0x%lx value=0x%lx\n",
            ret, feat_reg);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    ret = val_realm_rsi_features(1, &feat_reg);
    if ((ret != RSI_SUCCESS) || (feat_reg != 0)) {
        LOG(ERROR, "Invalid unsupported feature index result: ret=0x%lx value=0x%lx\n",
            ret, feat_reg);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    val_realm_return_to_host();
}
