/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "mm_common_host.h"
#include "val_host_sro.h"

#define RMI_RESULT_CANCEL(result) VAL_EXTRACT_BITS((result), 10, 10)

void mm_sro_cancel_observable_rejected_host(void)
{
    mm_reclaim_sro_ts sro;
    val_smc_param_ts cmd_ret;
    uint64_t ret;

    ret = mm_prepare_rec_destroy_reclaim_sro(&sro);
    if (ret == VAL_SKIP_CHECK)
    {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        mm_cleanup_reclaim_sro(&sro);
        return;
    }
    if (ret != VAL_SUCCESS)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        mm_cleanup_reclaim_sro(&sro);
        return;
    }

    if (RMI_RESULT_CANCEL(sro.args.x0) != RMI_OP_CANNOT_CANCEL)
    {
        LOG(ERROR, "Observable REC destroy SRO is cancelable, ret=0x%lx\n",
            sro.args.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        mm_cleanup_reclaim_sro(&sro);
        return;
    }

    cmd_ret = val_host_rmi_op_cancel_smc(sro.handle);
    if (cmd_ret.x0 != RMI_ERROR_INPUT)
    {
        LOG(ERROR, "RMI_OP_CANCEL accepted non-cancelable SRO, ret=0x%lx\n",
            cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        mm_cleanup_reclaim_sro(&sro);
        return;
    }

    ret = mm_complete_reclaim_sro(&sro);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "REC destroy SRO completion failed, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        mm_cleanup_reclaim_sro(&sro);
        return;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));
    mm_cleanup_reclaim_sro(&sro);
}
