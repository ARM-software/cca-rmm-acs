/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"

void mm_rsi_ripas_empty_protected_ipa_realm(void)
{
    val_realm_rsi_host_call_t *realm_host_call;
    val_smc_param_ts args;
    uint64_t ipa_base;
    uint64_t size;
    uint64_t flags = RSI_NO_CHANGE_DESTROYED;

    realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    ipa_base = realm_host_call->gprs[1];
    size = realm_host_call->gprs[2];

    args = val_realm_rsi_ipa_state_set(ipa_base, ipa_base + size, RSI_EMPTY, flags);
    if ((args.x0 != RSI_SUCCESS) || (args.x1 != (ipa_base + size)))
    {
        LOG(ERROR, "RSI_IPA_STATE_SET failed, ret=0x%lx out_top=0x%lx\n", args.x0, args.x1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    args = val_realm_rsi_ipa_state_get(ipa_base, ipa_base + size);
    if ((args.x0 != RSI_SUCCESS) || (args.x2 != RSI_EMPTY))
    {
        LOG(ERROR, "RSI_IPA_STATE_GET did not report RIPAS_EMPTY, ret=0x%lx ripas=0x%lx\n",
            args.x0, args.x2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    if (val_realm_rsi_realm_config(ipa_base) != RSI_ERROR_INPUT)
    {
        LOG(ERROR, "RSI_REALM_CONFIG did not fail for RIPAS_EMPTY Protected IPA\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    val_realm_return_to_host();
}
