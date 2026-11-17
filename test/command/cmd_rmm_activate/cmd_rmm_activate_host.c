/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"

void cmd_rmm_activate_host(void)
{
    val_smc_param_ts args;
    uint64_t ret;

    /*
     * The framework has already activated the RMM. Check that activating it
     * again returns RMI_ERROR_GLOBAL, as required when the RMM is not in INIT.
     */
    ret = val_host_rmi_rmm_activate();
    if (ret != RMI_ERROR_GLOBAL)
    {
        LOG(ERROR, "Unexpected RMI_RMM_ACTIVATE return: ret=0x%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    /* Check RMM state is ACTIVE. */
    args = val_host_rmi_rmm_state_get();
    if ((args.x0 != RMI_SUCCESS) ||
        (args.x1 != RMI_RMM_STATE_ACTIVE))
    {
        LOG(ERROR, "Unexpected RMM state: x0=0x%x x1=0x%x\n",
            args.x0, args.x1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
