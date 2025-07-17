/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"

#define CONTEXT_ID 0x5555

#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512 * L3_SIZE)
#define IPA_AUX_ASSIGNED 7 * PAGE_SIZE

void cmd_rtt_set_ripas_realm(void)
{
    val_smc_param_ts cmd_ret;
    uint64_t ret;
    /* Set RIPAS of the first 4KB granule to RAM */
    uint64_t ipa_base = 0x0000, ipa_top = 0x1000;

    if (val_get_primary_mpidr() != val_read_mpidr())
    {
        /* Executing IPA STATE_SET in Secodary VCPU to trigger base_align at walk.level = 2*/
        cmd_ret = val_realm_rsi_ipa_state_set(L2_SIZE + L3_SIZE, 2 * L2_SIZE, RSI_RAM,
                                                                 RSI_NO_CHANGE_DESTROYED);
        ret = cmd_ret.x0;
        if (ret) {
            LOG(ERROR, " IPA_STATE_SET failed with ret value: %d \n", ret);
            goto exit;
        }

        /* Execure RSI_SET_RIPAS again to trigger no_progress condition */
        cmd_ret = val_realm_rsi_ipa_state_set(L2_SIZE, 3 * L2_SIZE, RSI_RAM,
                                                                 RSI_NO_CHANGE_DESTROYED);
        ret = cmd_ret.x0;
        if (ret) {
            LOG(ERROR, " IPA_STATE_SET failed with ret value: %d \n", ret);
            val_realm_return_to_host();
        }

#if defined(RMM_V_1_1)
        ipa_base = IPA_AUX_ASSIGNED;
        /* Execure RSI_SET_RIPAS again to test aux_live */
        cmd_ret = val_realm_rsi_ipa_state_set(ipa_base, ipa_base + PAGE_SIZE, RSI_RAM,
                                                                 RSI_NO_CHANGE_DESTROYED);
        ret = cmd_ret.x0;
        if (ret) {
            LOG(ERROR, " IPA_STATE_SET failed with ret value: %d \n", ret);
            val_realm_return_to_host();
        }
#endif

        /* Execure RSI_SET_RIPAS again to compare top_gran_align < top_rtt_align */
        cmd_ret = val_realm_rsi_ipa_state_set(L2_SIZE, 3 * L2_SIZE, RSI_RAM,
                                                                 RSI_NO_CHANGE_DESTROYED);
        ret = cmd_ret.x0;
        if (ret) {
            LOG(ERROR, " IPA_STATE_SET failed with ret value: %d \n", ret);
            val_realm_return_to_host();
        }
    }

    /* Power on REC[1] for future execution */
    ret = val_psci_cpu_on(REC_NUM(1), val_realm_get_secondary_cpu_entry(), CONTEXT_ID);
    if (ret)
    {
        LOG(ERROR, "PSCI CPU ON failed with ret status : 0x%x \n", ret);
        goto exit;
    }

    /* Executing IPA_STATE_SET in Primary vCPU to return valid base, top */
    cmd_ret = val_realm_rsi_ipa_state_set(ipa_base, ipa_top, RSI_RAM, RSI_NO_CHANGE_DESTROYED);
    ret = cmd_ret.x0;
    if (ret) {
        LOG(ERROR, " IPA_STATE_SET failed with ret value: %d \n", ret);
        goto exit;
    }

exit:
    val_realm_return_to_host();

}
