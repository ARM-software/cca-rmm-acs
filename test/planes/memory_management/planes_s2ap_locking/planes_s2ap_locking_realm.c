/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_planes.h"

#define PERM_INDEX_0      0
#define PERM_INDEX_1      1

void planes_s2ap_locking_realm(void)
{
    val_smc_param_ts cmd_ret;
    uint64_t p1_ipa_base, p1_ipa_top;

    /* At Realm activatio S2AP overlay index 0 is LOCKED
     * Test intent : Attempt to set plane index, overlay index tuple (1,0) to
     * hold value RW and expect the command to fail */
    cmd_ret = val_realm_rsi_mem_set_perm_value(PLANE_1_INDEX, PERM_INDEX_0, S2_AP_RW);

    if (cmd_ret.x0 != RSI_ERROR_INPUT)
    {
        LOG(ERROR, "Unexpected output for RSI_MEM_SET_PERM_VALUE, ret = %d\n", cmd_ret.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    /* At Relam activation S2AP overly index 1-14 are UNLOCKED
     * Test intent : Set plane index , overlay index tuple (1, 1) as value RW_upX for
     * P1 IPA range, commands RSI_MEM_SET_PERM_(VALUE,INDEX) should suceed */

    p1_ipa_base = VAL_PLANE1_IMAGE_BASE_IPA;
    p1_ipa_top = p1_ipa_base + PLATFORM_REALM_IMAGE_SIZE;

    if (val_realm_plane_perm_init(PLANE_1_INDEX, PERM_INDEX_1, p1_ipa_base, p1_ipa_top))
    {
        LOG(ERROR, "Unexpected output for RSI_MEM_SET_PERM_{VALUE,INDEX}", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    /* Successful command execution of RSI_MEM_SET_PERM_INDEX locks the the overlay index */
    cmd_ret = val_realm_rsi_mem_set_perm_value(PLANE_1_INDEX, PERM_INDEX_1, S2_AP_RW);

    if (cmd_ret.x0 != RSI_ERROR_INPUT)
    {
        LOG(ERROR, "Unexpected output for RSI_MEM_SET_PERM_VALUE, ret = %d\n", cmd_ret.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}

