/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "mm_common_host.h"
#include "val_host_sro.h"

#define RMI_RESULT_CANCEL(result) VAL_EXTRACT_BITS((result), 10, 10)

void mm_sro_cancel_donate_request_host(void)
{
    mm_realm_create_sro_ts sro;
    val_smc_param_ts cmd_ret;
    uint64_t handle;

    if (mm_prepare_realm_create_sro(&sro))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        mm_cleanup_realm_create_sro(&sro);
        return;
    }

    cmd_ret = val_host_rmi_realm_create_smc(sro.realm.rd, sro.params_addr);
    if ((RMI_STATUS(cmd_ret.x0) != RMI_INCOMPLETE) ||
        (VAL_HOST_SRO_MEM_REQ(cmd_ret.x0) != RMI_OP_MEM_REQ_DONATE))
    {
        LOG(TEST, "Skipping: RMI_REALM_CREATE did not request donation, ret=0x%lx\n",
            cmd_ret.x0);
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        mm_cleanup_realm_create_sro(&sro);
        return;
    }

    if (RMI_RESULT_CANCEL(cmd_ret.x0) != RMI_OP_CAN_CANCEL)
    {
        LOG(ERROR, "Donation-request SRO is not cancelable, ret=0x%lx\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        if (val_host_sro_complete(&cmd_ret) == RMI_SUCCESS)
            sro.realm_created = 1;
        mm_cleanup_realm_create_sro(&sro);
        return;
    }

    handle = cmd_ret.x1;
    cmd_ret = val_host_rmi_op_cancel_smc(handle);
    if ((RMI_STATUS(cmd_ret.x0) != RMI_INCOMPLETE) ||
        (VAL_HOST_SRO_MEM_REQ(cmd_ret.x0) != RMI_OP_MEM_REQ_NONE) ||
        (cmd_ret.x1 != handle))
    {
        LOG(ERROR, "RMI_OP_CANCEL did not return no-memory incomplete result, ret=0x%lx\n",
            cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        (void)val_host_sro_complete_with_handle(&cmd_ret, handle);
        mm_cleanup_realm_create_sro(&sro);
        return;
    }

    if (VAL_HOST_SRO_MEM_REQ(cmd_ret.x0) == RMI_OP_MEM_REQ_DONATE)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        (void)val_host_sro_complete_with_handle(&cmd_ret, handle);
        mm_cleanup_realm_create_sro(&sro);
        return;
    }

    cmd_ret = val_host_rmi_op_continue_smc(handle, RMI_CONTINUE_KEEP_GOING);
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "Cancelled SRO continue failed, ret=0x%lx\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        mm_cleanup_realm_create_sro(&sro);
        return;
    }

    cmd_ret = val_host_rmi_op_cancel_smc(handle);
    if (cmd_ret.x0 != RMI_ERROR_INPUT)
    {
        LOG(ERROR, "Cancelled SRO handle is still valid, ret=0x%lx\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        mm_cleanup_realm_create_sro(&sro);
        return;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));
    mm_cleanup_realm_create_sro(&sro);
}
