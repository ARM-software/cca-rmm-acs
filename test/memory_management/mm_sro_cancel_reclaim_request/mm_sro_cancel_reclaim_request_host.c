/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "mm_common_host.h"
#include "val_host_sro.h"

#define RMI_RESULT_CANCEL(result) VAL_EXTRACT_BITS((result), 10, 10)

void mm_sro_cancel_reclaim_request_host(void)
{
    mm_realm_create_sro_ts sro;
    mm_sro_donation_ts donation;
    val_smc_param_ts cmd_ret;
    uint64_t handle;
    uint64_t ret;

    val_memset(&donation, 0, sizeof(donation));
    if (mm_prepare_realm_create_sro(&sro))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto out;
    }

    cmd_ret = val_host_rmi_realm_create_smc(sro.realm.rd, sro.params_addr);
    if ((RMI_STATUS(cmd_ret.x0) != RMI_INCOMPLETE) ||
        (VAL_HOST_SRO_MEM_REQ(cmd_ret.x0) != RMI_OP_MEM_REQ_DONATE) ||
        (RMI_RESULT_CANCEL(cmd_ret.x0) != RMI_OP_CAN_CANCEL))
    {
        LOG(TEST, "Skipping: no cancelable donation SRO, ret=0x%lx\n", cmd_ret.x0);
        if (RMI_STATUS(cmd_ret.x0) == RMI_INCOMPLETE)
        {
            if (val_host_sro_complete(&cmd_ret) == RMI_SUCCESS)
                sro.realm_created = 1;
        }
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto out;
    }
    handle = cmd_ret.x1;

    if (mm_prepare_sro_donation_list(cmd_ret.x2, &donation))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto cancel_sro;
    }

    cmd_ret = val_host_rmi_op_mem_donate_smc(handle, donation.list_addr, 1);
    if (RMI_STATUS(cmd_ret.x0) != RMI_INCOMPLETE)
    {
        LOG(ERROR, "RMI_OP_MEM_DONATE failed, ret=0x%lx\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto cancel_sro;
    }
    donation.mem_consumed = 1;

    cmd_ret = val_host_rmi_op_cancel_smc(handle);
    if (RMI_STATUS(cmd_ret.x0) != RMI_INCOMPLETE)
    {
        LOG(ERROR, "RMI_OP_CANCEL failed after donation, ret=0x%lx\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto out;
    }

    if (VAL_HOST_SRO_MEM_REQ(cmd_ret.x0) == RMI_OP_MEM_REQ_DONATE)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto out;
    }

    if (VAL_HOST_SRO_MEM_REQ(cmd_ret.x0) != RMI_OP_MEM_REQ_RECLAIM)
    {
        LOG(TEST, "Skipping: cancelled SRO did not require reclaim, ret=0x%lx\n",
            cmd_ret.x0);
        ret = val_host_sro_complete_with_handle(&cmd_ret, handle);
        if (ret != RMI_SUCCESS)
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        else
            val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto out;
    }

    if (cmd_ret.x1 != handle)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto out;
    }

    ret = val_host_sro_complete_with_handle(&cmd_ret, handle);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "Cancelled reclaim SRO completion failed, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto out;
    }

    cmd_ret = val_host_rmi_op_cancel_smc(handle);
    if (cmd_ret.x0 != RMI_ERROR_INPUT)
    {
        LOG(ERROR, "Cancelled SRO handle is still valid, ret=0x%lx\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto out;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));
    goto out;

cancel_sro:
    cmd_ret = val_host_rmi_op_cancel_smc(handle);
    if (RMI_STATUS(cmd_ret.x0) == RMI_INCOMPLETE)
        (void)val_host_sro_complete_with_handle(&cmd_ret, handle);

out:
    mm_release_sro_donation_list(&donation);
    mm_cleanup_realm_create_sro(&sro);
}
