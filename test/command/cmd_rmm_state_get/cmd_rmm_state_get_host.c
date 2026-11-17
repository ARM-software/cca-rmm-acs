/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_host_rmi.h"

void cmd_rmm_state_get_host(void)
{
    val_smc_param_ts cmd_ret;

    cmd_ret = val_host_rmi_rmm_state_get();
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_RMM_STATE_GET failed, ret: 0x%x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    /* RMM State should be Active */
    if (VAL_EXTRACT_BITS(cmd_ret.x1, 0, 7) != RMI_RMM_STATE_ACTIVE)
    {
        LOG(ERROR, "Unexpected RMM state 0x%x\n", cmd_ret.x1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return;
    }

    if (VAL_EXTRACT_BITS(cmd_ret.x1, 8, 63) != 0)
    {
        LOG(ERROR, "Reserved RMM state bits are MBZ : 0x%x\n", cmd_ret.x1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        return;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));
}
