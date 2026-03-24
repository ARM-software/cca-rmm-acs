/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_rhi.h"

static uint64_t rhi_rec_enter(val_host_realm_ts *realm)
{
    uint64_t ret;

    ret = val_host_rmi_rec_enter(realm->rec[0], realm->run[0]);
    if (ret)
    {
        LOG(ERROR, "REC enter failed, ret=%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return VAL_ERROR;
    } else if (val_host_check_realm_exit_rsi_host_call((val_host_rec_run_ts *)realm->run[0]))
    {
        LOG(ERROR, "REC_EXIT: HOST_CALL params mismatch\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        return VAL_ERROR;
    }

    ret = val_host_rhi_dispatch(realm);
    if (ret)
    {
        LOG(ERROR, "RHI command failed, ret=%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void rhi_session_receive_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;
    uint64_t i;
    const char *step_desc[] = {
        "RHI_SESSION_OPEN: blocking",
        "RHI_SESSION_RECEIVE: size query",
        "RHI_SESSION_RECEIVE: size 0",
        "RHI_SESSION_RECEIVE: blocking",
        "RHI_SESSION_CLOSE: blocking",
        "RHI_SESSION_RECEIVE: invalid session id",
        "RHI_SESSION_OPEN: non-blocking (initial)",
        "RHI_SESSION_RECEIVE: invalid state (in progress)",
        "RHI_SESSION_OPEN: non-blocking (poll to established)",
        "RHI_SESSION_RECEIVE: misaligned buffer",
        "RHI_SESSION_RECEIVE: non-blocking (in progress)",
        "RHI_SESSION_RECEIVE: non-blocking (complete)",
        "RHI_SESSION_CLOSE: non-blocking (in progress)",
        "RHI_SESSION_CLOSE: non-blocking (complete)"
    };

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    /* Populate realm with one REC */
    if (val_host_realm_setup(&realm, true))
    {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    for (i = 0; i < (sizeof(step_desc) / sizeof(step_desc[0])); i++)
    {
        LOG(ALWAYS, "%s\n", step_desc[i]);
        ret = rhi_rec_enter(&realm);
        if (ret)
            goto destroy_realm;
    }

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "REC enter failed, ret=%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
