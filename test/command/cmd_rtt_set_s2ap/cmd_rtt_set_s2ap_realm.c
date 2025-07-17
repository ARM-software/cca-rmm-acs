/*
 * Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"
#include "val_realm_planes.h"

#define CONTEXT_ID 0x5555

#define NUM_AUX_PLANES      2
#define PLANE_INDEX_        1
#define PLANE_INDEX_2       2
#define PERMISSION_INDEX_1  1
#define PERMISSION_INDEX_2  2
#define PERMISSION_INDEX_3  3

#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512 * L3_SIZE)

void cmd_rtt_set_s2ap_realm(void)
{
    val_smc_param_ts cmd_ret;
    uint64_t ret, i;
    uint64_t ipa_base = 0x0000, ipa_top = 0x1000;
    uint64_t base, cookie_value = 0;

    if (val_get_primary_mpidr() != val_read_mpidr())
    {
        cookie_value = 0;
        base = L2_SIZE + L3_SIZE;

        /* Set Overlay value RW_upX for (plane_idx, perm_idx) = (2, 2) */
        cmd_ret = val_realm_rsi_mem_set_perm_value(PLANE_INDEX_2, PERMISSION_INDEX_2, S2_AP_RW_upX);
        if (cmd_ret.x0) {
            LOG(ERROR, "MEM_SET_PERM_VALUE failed with : %d \n", cmd_ret.x0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto exit;
        }

        /* Initiate S2AP change to trigger base_align_pri failure condition */
        cmd_ret = val_realm_rsi_mem_set_perm_index(base, 2 * L2_SIZE,
                                                         PERMISSION_INDEX_2, cookie_value);
        if (cmd_ret.x0 == RSI_ERROR_INPUT)
        {
            LOG(ERROR, "MEM_SET_PERM_INDEX failed with : 0x%lx , Response %d \n",
                                                             cmd_ret.x0, cmd_ret.x2);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }


        /* Set Overlay value RW_upX for (plane_idx, perm_idx) = (1, 3) and (2, 3) */
        for (i = 0; i < NUM_AUX_PLANES; i++)
        {
            cmd_ret = val_realm_rsi_mem_set_perm_value(i + 1, PERMISSION_INDEX_3, S2_AP_RW_upX);
            if (cmd_ret.x0) {
                LOG(ERROR, "MEM_SET_PERM_VALUE failed with : %d \n", cmd_ret.x0);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
                goto exit;
            }
        }

        /* Initiate S2AP change to trigger base_align_aux failure condition */
        cookie_value = 0;
        base = L2_SIZE + L3_SIZE;

        while (base != ipa_top) {
            cmd_ret = val_realm_rsi_mem_set_perm_index(base, 2 * L2_SIZE,
                                                               PERMISSION_INDEX_3, cookie_value);
            if (cmd_ret.x0 == RSI_ERROR_INPUT)
            {
                LOG(ERROR, "MEM_SET_PERM_INDEX failed with : 0x%lx , Response %d \n",
                                                                 cmd_ret.x0, cmd_ret.x2);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
                goto exit;
            }

            base = cmd_ret.x1;
            cookie_value = cmd_ret.x3;
        }

    }

    /* Power on REC[1] for future execution */
    ret = val_psci_cpu_on(REC_NUM(1), val_realm_get_secondary_cpu_entry(), CONTEXT_ID);
    if (ret)
    {
        LOG(ERROR, "PSCI CPU ON failed with ret status : 0x%x \n", ret);
        goto exit;
    }

    /* Set Overlay value RW_upX for (plane_idx, perm_idx) = (1, 1) and (2, 1) */
    for (i = 0; i < NUM_AUX_PLANES; i++)
    {
        cmd_ret = val_realm_rsi_mem_set_perm_value(i + 1, PERMISSION_INDEX_1, S2_AP_RW_upX);
        if (cmd_ret.x0) {
            LOG(ERROR, "MEM_SET_PERM_VALUE failed with : %d \n", cmd_ret.x0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto exit;
        }
    }

    base = ipa_base;
    /* Initiate S2AP change in Primary vCPU to return valid base, top */
    while (base != ipa_top) {
        cmd_ret = val_realm_rsi_mem_set_perm_index(base, ipa_top, PERMISSION_INDEX_1, cookie_value);
        if (cmd_ret.x0 == RSI_ERROR_INPUT)
        {
            LOG(ERROR, "MEM_SET_PERM_INDEX failed with : 0x%lx , Response %d \n",
                                                             cmd_ret.x0, cmd_ret.x2);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
            goto exit;
        }

        base = cmd_ret.x1;
        cookie_value = cmd_ret.x3;
    }

exit:
    val_realm_return_to_host();

}
