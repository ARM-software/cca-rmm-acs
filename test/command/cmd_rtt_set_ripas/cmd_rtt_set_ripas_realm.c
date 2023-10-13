/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"

#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512 * L3_SIZE)

void cmd_rtt_set_ripas_realm(void)
{
    val_smc_param_ts cmd_ret;
    uint64_t ret;
    /* Set RIPAS of the first 4KB granule to RAM */
    uint64_t ipa_base = 0x0000, ipa_top = 0x1000;

    LOG(TEST, "\tIn realm, Excecuting RSI_IPA_STATE_SET\n", 0, 0);

    /* Executing IPA STATE_SET in Secodary VCPU to trigger base_align at walk.level = 2*/
    if (val_get_primary_mpidr() != val_read_mpidr())
    {
        cmd_ret = val_realm_rsi_ipa_state_set(ipa_base + L2_SIZE, ipa_top + L2_SIZE, RSI_RAM,
                                                                     RSI_NO_CHANGE_DESTROYED);
        ret = cmd_ret.x0;
        if (ret) {
            LOG(ERROR, "\t IPA_STATE_SET failed with ret value: %d \n", ret, 0);
            val_realm_return_to_host();
        }
    }

    /* Executing IPA_STATE_SET in Primary vCPU to return valid base, top */
    cmd_ret = val_realm_rsi_ipa_state_set(ipa_base, ipa_top, RSI_RAM, RSI_NO_CHANGE_DESTROYED);
    ret = cmd_ret.x0;
    if (ret) {
        LOG(ERROR, "\t IPA_STATE_SET failed with ret value: %d \n", ret, 0);
        val_realm_return_to_host();
    }
}
