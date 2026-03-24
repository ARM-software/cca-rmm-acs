/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_rhi.h"

static uint64_t validate_rhi_session_open(val_host_realm_ts *realm)
{
    uint64_t ret;
    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm->rec[0], realm->run[0]);
    if (ret)
    {
        LOG(ERROR, "REC enter failed, ret=%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return VAL_ERROR;
    } else if (val_host_check_realm_exit_rsi_host_call((val_host_rec_run_ts *)realm->run[0]))
    {
        LOG(ERROR, "REC_EXIT: HOST_CALL params mismatch\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return VAL_ERROR;
    }

    /* Process the RHI command from realm and update rec enter structure */
    ret = val_host_rhi_dispatch(realm);
    if (ret)
    {
        LOG(ERROR, "RHI command failed, ret=%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void rhi_session_open_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;
    uint64_t i;
    const char *step_desc[] = {
        "Validate RHI_SESSION_OPEN: invalid session id",
        "Validate RHI_SESSION_OPEN: unknown connection type",
        "Validate RHI_SESSION_OPEN: blocking success",
        "Validate RHI_SESSION_OPEN: invalid protocol state operation",
        "Validate RHI_SESSION_CLOSE: close after blocking open",
        "Validate RHI_SESSION_OPEN: non-blocking in-progress",
        "Validate RHI_SESSION_OPEN: non-blocking invalid id",
        "Validate RHI_SESSION_OPEN: non-blocking success",
        "Validate RHI_SESSION_CLOSE: close after non-blocking open",
        "Validate RHI_SESSION_CLOSE: close again"
    };

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    /* Populate realm with one REC */
    if (val_host_realm_setup(&realm, true))
    {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    for (i = 0; i < (sizeof(step_desc) / sizeof(step_desc[0])); i++)
    {
        LOG(ALWAYS, "%s\n", step_desc[i]);
        ret = validate_rhi_session_open(&realm);
        if (ret)
            goto destroy_realm;
    }

    /* Enter REC[0]  */
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
