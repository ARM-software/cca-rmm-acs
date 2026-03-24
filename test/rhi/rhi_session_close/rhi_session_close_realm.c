/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_rhi.h"
#include "val_realm_framework.h"
#include "rhi_session_close.h"

#define RHI_INVALID_SESSION_ID 0xFFFFFFFF
static uint64_t g_session_id;

static uint64_t validate_session_close(uint64_t sess_id, uint64_t status_code)
{
    uint64_t ret;
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t rhi_call = {0};

    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_SESSION_CLOSE;
    rhi_call.gprs[1] = sess_id;
    ret = val_realm_rsi_rhi_host(&rhi_call);
    if (ret)
    {
        LOG(ERROR, "RSI_HOST_CALL failed: %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return VAL_ERROR;
    }

    switch (status_code)
    {
        case RHI_SESS_INVALID_SESSION_ID:
            if (rhi_call.gprs[0] != RHI_SESS_INVALID_SESSION_ID)
            {
                LOG(ERROR, "RHI_SESSION_CLOSE: invalid session_id failed err: %lx\n",
                                                                        rhi_call.gprs[0]);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
                return VAL_ERROR;
            }
            break;
        case RHI_SESS_SUCCESS:
            if ((rhi_call.gprs[0] != RHI_SESS_SUCCESS) ||
                (rhi_call.gprs[2] != RHI_HSS_SESSION_UNCONNECTED))
            {
                LOG(ERROR, "RHI_SESSION_CLOSE: failed err: %lx state %lx\n",
                                                        rhi_call.gprs[0], rhi_call.gprs[2]);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
                return VAL_ERROR;
            }
            break;
        case RHI_HSS_CONNECTION_CLOSE_IN_PROGRESS:
            if ((rhi_call.gprs[0] != RHI_SESS_SUCCESS) ||
                (rhi_call.gprs[2] != RHI_HSS_CONNECTION_CLOSE_IN_PROGRESS))
            {
                LOG(ERROR, "RHI_SESSION_CLOSE: failed err: %lx state %lx\n",
                                                        rhi_call.gprs[0], rhi_call.gprs[2]);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
                return VAL_ERROR;
            }
            g_session_id = rhi_call.gprs[1];
            break;
    }

    return VAL_SUCCESS;
}

static uint64_t session_open(uint64_t sess_id)
{
    uint64_t ret;
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t rhi_call = {0};

    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_SESSION_OPEN;
    rhi_call.gprs[1] = sess_id;
    rhi_call.gprs[2] = RHI_CONN_MODE_NON_BLOCKING;
    ret = val_realm_rsi_rhi_host(&rhi_call);
    if (ret)
    {
        LOG(ERROR, "RSI_HOST_CALL failed: %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        return VAL_ERROR;
    }

    if (rhi_call.gprs[0] != RHI_SESS_SUCCESS)
    {
        LOG(ERROR, "RHI_SESSION_OPEN: failed err: %lx state: %lx\n",
                                            rhi_call.gprs[0], rhi_call.gprs[2]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        return VAL_ERROR;
    }
    g_session_id = rhi_call.gprs[1];

    return VAL_SUCCESS;
}

void rhi_session_close_realm(void)
{
    uint64_t ret;
    uint64_t i;
    const uint64_t session_id_current = ~0ULL;

    const struct session_step steps[] = {
        {ACTION_OPEN, 0x0, 0x0},
        {ACTION_OPEN, session_id_current, 0x0},
        {ACTION_CLOSE, RHI_INVALID_SESSION_ID, RHI_SESS_INVALID_SESSION_ID},
        {ACTION_CLOSE, session_id_current, RHI_HSS_CONNECTION_CLOSE_IN_PROGRESS},
        {ACTION_CLOSE, session_id_current, RHI_SESS_SUCCESS}
    };

    for (i = 0; i < (sizeof(steps) / sizeof(steps[0])); i++)
    {
        uint64_t sess_id = steps[i].sess_id;
        if (sess_id == session_id_current)
            sess_id = g_session_id;

        if (steps[i].action == ACTION_OPEN) {
            ret = session_open(sess_id);
        } else {
            ret = validate_session_close(sess_id, steps[i].expected);
        }

        if (ret)
            goto exit;
    }

exit:
    val_realm_return_to_host();
}
