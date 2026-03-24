/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"

void cmd_vdev_map_realm(void)
{
    uint64_t ipa_base = 0, size = 0;
    uint8_t ripas_val = 0;
    val_realm_rsi_host_call_t *realm_host_call = NULL;
    val_smc_param_ts args = {0,};
    uint32_t status = VAL_SUCCESS;
    uint64_t flags = RSI_NO_CHANGE_DESTROYED;

    realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);

    ipa_base = realm_host_call->gprs[1];
    size = realm_host_call->gprs[2];
    ripas_val = RSI_RAM;
    val_memset(&args, 0x0, sizeof(val_smc_param_ts));

    args = val_realm_rsi_ipa_state_set(ipa_base, ipa_base + size, ripas_val, flags);
    if (args.x0 || (args.x1 != (ipa_base + size)))
    {
        LOG(ERROR, "rsi_ipa_state_set failed x0 %lx x1 %lx\n", args.x0, args.x1);
        status = VAL_ERROR_POINT(1);
    }

    if (status)
    {
        val_set_status(RESULT_FAIL(status));
    }

    val_realm_return_to_host();
}
