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
#include "rhi_session_open.h"

#define RHI_INVALID_SESSION_ID 0xFFFFFFFF
static uint64_t g_session_id;

static uint64_t validate_session_open(uint64_t sess_id,
                                               uint64_t conn_type, uint64_t status_code)
{
    uint64_t ret;
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t rhi_call = {0};

    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_SESSION_OPEN;
    rhi_call.gprs[1] = sess_id;
    rhi_call.gprs[2] = conn_type;
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
                LOG(ERROR, "RHI_SESSION_OPEN: invalid session_id failed err: %lx\n",
                                                                        rhi_call.gprs[0]);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
                return VAL_ERROR;
            }
            break;
        case RHI_SESS_CONNECTION_TYPE_NOT_SUPPORTED:
            if (rhi_call.gprs[0] != RHI_SESS_CONNECTION_TYPE_NOT_SUPPORTED)
            {
                LOG(ERROR, "RHI_SESSION_OPEN: Unknown connection type err: %lx\n",
                                                                        rhi_call.gprs[0]);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
                return VAL_ERROR;
            }
            break;
        case RHI_SESS_SUCCESS:
            if ((rhi_call.gprs[0] != RHI_SESS_SUCCESS) ||
                (rhi_call.gprs[2] != RHI_HSS_CONNECTION_ESTABLISHED))
            {
                if (conn_type == RHI_CONN_MODE_NON_BLOCKING)
                {
                    LOG(ERROR, "RHI_SESSION_OPEN: Non-blocking failed err: %lx state %lx\n",
                                                            rhi_call.gprs[0], rhi_call.gprs[2]);
                } else {
                    LOG(ERROR, "RHI_SESSION_OPEN: Blocking failed err: %lx state %lx\n",
                                                            rhi_call.gprs[0], rhi_call.gprs[2]);
                }
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
                return VAL_ERROR;
            }
            g_session_id = rhi_call.gprs[1];
            break;
        case RHI_SESS_INVALID_STATE_FOR_OPERATION:
            if (rhi_call.gprs[0] != RHI_SESS_INVALID_STATE_FOR_OPERATION)
            {
                LOG(ERROR, "RHI_SESSION_OPEN: Invalid protocol state operation err: %lx\n",
                                                                        rhi_call.gprs[0]);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
                return VAL_ERROR;
            }
            break;
        case RHI_HSS_CONNECTION_IN_PROGRESS:
            if ((rhi_call.gprs[0] != RHI_SESS_SUCCESS) ||
                (rhi_call.gprs[2] != RHI_HSS_CONNECTION_IN_PROGRESS))
            {
                LOG(ERROR, "RHI_SESSION_OPEN: Non-blocking failed err: %lx state %lx\n",
                                                            rhi_call.gprs[0], rhi_call.gprs[2]);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
                return VAL_ERROR;
            }
            g_session_id = rhi_call.gprs[1];
            break;
    }

    return VAL_SUCCESS;
}

static uint64_t session_close(void)
{
    uint64_t ret;
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t rhi_call = {0};

    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_SESSION_CLOSE;
    rhi_call.gprs[1] = g_session_id;
    ret = val_realm_rsi_rhi_host(&rhi_call);
    if (ret)
    {
        LOG(ERROR, "RSI_HOST_CALL failed: %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        return VAL_ERROR;
    }

    if (rhi_call.gprs[0] != RHI_SESS_SUCCESS)
    {
        LOG(ERROR, "RHI_SESSION_CLOSE: Blocking failed err: %lx state: %lx\n",
                                                rhi_call.gprs[0], rhi_call.gprs[2]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void rhi_session_open_realm(void)
{
    uint64_t ret;
    uint64_t i;

    const uint64_t session_id_current = ~0ULL;

    const struct session_step steps[] = {
        {ACTION_OPEN, 0x1, RHI_CONN_MODE_BLOCKING,
            RHI_SESS_INVALID_SESSION_ID, "invalid session id"},
        {ACTION_OPEN, 0x0, RHI_CONN_MODE_BLOCKING | RHI_CONN_MODE_NON_BLOCKING,
            RHI_SESS_CONNECTION_TYPE_NOT_SUPPORTED, "unknown connection type"},
        {ACTION_OPEN, 0x0, RHI_CONN_MODE_BLOCKING,
            RHI_SESS_SUCCESS, "blocking success"},
        {ACTION_OPEN, session_id_current, RHI_CONN_MODE_BLOCKING,
            RHI_SESS_INVALID_STATE_FOR_OPERATION, "invalid protocol state operation"},
        {ACTION_CLOSE, 0x0, 0x0, 0x0, "close after blocking open"},
        {ACTION_OPEN, 0x0, RHI_CONN_MODE_NON_BLOCKING,
            RHI_HSS_CONNECTION_IN_PROGRESS, "non-blocking in-progress"},
        {ACTION_OPEN, RHI_INVALID_SESSION_ID, RHI_CONN_MODE_NON_BLOCKING,
            RHI_SESS_INVALID_SESSION_ID, "non-blocking invalid id"},
        {ACTION_OPEN, session_id_current, RHI_CONN_MODE_NON_BLOCKING,
            RHI_SESS_SUCCESS, "non-blocking success"},
        {ACTION_CLOSE, 0x0, 0x0, 0x0, "close after non-blocking open"},
        {ACTION_CLOSE, 0x0, 0x0, 0x0, "close again"}
    };

    for (i = 0; i < (sizeof(steps) / sizeof(steps[0])); i++)
    {
        if (steps[i].action == ACTION_OPEN) {
            uint64_t sess_id = steps[i].sess_id;
            if (sess_id == session_id_current)
                sess_id = g_session_id;
            ret = validate_session_open(sess_id,
                                        steps[i].conn_type,
                                        steps[i].expected);
        } else {
            ret = session_close();
        }

        if (ret)
            goto exit;
    }

exit:
    val_realm_return_to_host();
}
