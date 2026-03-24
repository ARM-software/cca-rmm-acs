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
#include "rhi_session_send.h"

#define SEND_DATA_PATTERN 0xabcdabcdU
#define SEND_DATA_LENGTH 64U
#define RHI_INVALID_SESSION_ID 0xFFFFFFFF

static uint64_t g_session_id;

static void fill_send_buffer(uint64_t buffer_ipa)
{
    uint32_t *buffer = (uint32_t *)buffer_ipa;
    uint32_t count = SEND_DATA_LENGTH / sizeof(uint32_t);
    uint32_t idx;

    for (idx = 0; idx < count; idx++)
    {
        buffer[idx] = SEND_DATA_PATTERN;
    }
}

static uint64_t session_open(uint64_t sess_id, uint64_t conn_type,
                             uint64_t expected_state)
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

    if (rhi_call.gprs[0] != RHI_SESS_SUCCESS ||
        rhi_call.gprs[2] != expected_state)
    {
        LOG(ERROR, "RHI_SESSION_OPEN failed: %lx state %lx\n",
                                                rhi_call.gprs[0], rhi_call.gprs[2]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return VAL_ERROR;
    }

    g_session_id = rhi_call.gprs[1];
    return VAL_SUCCESS;
}

static uint64_t session_send(uint64_t sess_id, uint64_t buffer_ipa,
                             uint64_t length, uint64_t expected_status,
                             uint64_t expected_state, uint64_t expected_len)
{
    uint64_t ret;
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t rhi_call = {0};

    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_SESSION_SEND;
    rhi_call.gprs[1] = sess_id;
    rhi_call.gprs[2] = buffer_ipa;
    rhi_call.gprs[3] = length;
    rhi_call.gprs[4] = 0;
    ret = val_realm_rsi_rhi_host(&rhi_call);
    if (ret)
    {
        LOG(ERROR, "RSI_HOST_CALL failed: %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        return VAL_ERROR;
    }

    if (expected_status != RHI_SESS_SUCCESS)
    {
        if (rhi_call.gprs[0] != expected_status)
        {
            LOG(ERROR, "RHI_SESSION_SEND failed: %lx\n", rhi_call.gprs[0]);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            return VAL_ERROR;
        }
        return VAL_SUCCESS;
    }

    if (rhi_call.gprs[0] != RHI_SESS_SUCCESS ||
        rhi_call.gprs[2] != expected_state ||
        rhi_call.gprs[3] != expected_len)
    {
        LOG(ERROR, "RHI_SESSION_SEND failed: %lx state %lx len %lx\n",
            rhi_call.gprs[0], rhi_call.gprs[2], rhi_call.gprs[3]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t session_close(uint64_t sess_id, uint64_t expected_state)
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
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        return VAL_ERROR;
    }

    if (rhi_call.gprs[0] != RHI_SESS_SUCCESS ||
        rhi_call.gprs[2] != expected_state)
    {
        LOG(ERROR, "RHI_SESSION_CLOSE failed: %lx state %lx\n",
                                                rhi_call.gprs[0], rhi_call.gprs[2]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void rhi_session_send_realm(void)
{
    uint64_t ret;
    uint64_t buffer_ipa;
    uint64_t buffer_ipa_unaligned;
    uint64_t i;
    const uint64_t session_id_current = ~0ULL;

    buffer_ipa = (uint64_t)(val_get_shared_region_base() + TEST_USE_OFFSET1);
    buffer_ipa_unaligned = buffer_ipa + 8;
    val_memset((void *)buffer_ipa, 0, PAGE_SIZE);
    fill_send_buffer(buffer_ipa);

    const struct session_step steps[] = {
        {ACTION_OPEN, 0x0, RHI_CONN_MODE_BLOCKING,
            RHI_SESS_SUCCESS, RHI_HSS_CONNECTION_ESTABLISHED, 0},
        {ACTION_SEND, session_id_current, 0,
            RHI_SESS_SUCCESS, RHI_HSS_IO_COMPLETE, SEND_DATA_LENGTH},
        {ACTION_CLOSE, session_id_current, 0,
            RHI_SESS_SUCCESS, RHI_HSS_SESSION_UNCONNECTED, 0},
        {ACTION_SEND, RHI_INVALID_SESSION_ID, 0,
            RHI_SESS_INVALID_SESSION_ID, 0, 0},
        {ACTION_OPEN, 0x0, RHI_CONN_MODE_NON_BLOCKING,
            RHI_SESS_SUCCESS, RHI_HSS_CONNECTION_IN_PROGRESS, 0},
        {ACTION_SEND, session_id_current, 0,
            RHI_SESS_INVALID_STATE_FOR_OPERATION, 0, 0},
        {ACTION_OPEN, session_id_current, RHI_CONN_MODE_NON_BLOCKING,
            RHI_SESS_SUCCESS, RHI_HSS_CONNECTION_ESTABLISHED, 0},
        {ACTION_SEND, session_id_current, 0,
            RHI_SESS_ACCESS_FAILED, 0, 0},
        {ACTION_SEND, session_id_current, 0,
            RHI_SESS_SUCCESS, RHI_HSS_IO_IN_PROGRESS, 0},
        {ACTION_SEND, session_id_current, 0,
            RHI_SESS_SUCCESS, RHI_HSS_IO_COMPLETE, SEND_DATA_LENGTH},
        {ACTION_CLOSE, session_id_current, 0,
            RHI_SESS_SUCCESS, RHI_HSS_CONNECTION_CLOSE_IN_PROGRESS, 0},
        {ACTION_CLOSE, session_id_current, 0,
            RHI_SESS_SUCCESS, RHI_HSS_SESSION_UNCONNECTED, 0}
    };

    for (i = 0; i < (sizeof(steps) / sizeof(steps[0])); i++)
    {
        uint64_t sess_id = steps[i].sess_id;
        if (sess_id == session_id_current)
            sess_id = g_session_id;

        if (steps[i].action == ACTION_OPEN) {
            ret = session_open(sess_id, steps[i].conn_type,
                               steps[i].expected_state);
        } else if (steps[i].action == ACTION_SEND) {
            uint64_t send_ipa = buffer_ipa;
            if (steps[i].expected_status == RHI_SESS_ACCESS_FAILED)
                send_ipa = buffer_ipa_unaligned;
            ret = session_send(sess_id, send_ipa, SEND_DATA_LENGTH,
                               steps[i].expected_status,
                               steps[i].expected_state, steps[i].expected_len);
        } else {
            ret = session_close(sess_id, steps[i].expected_state);
        }

        if (ret)
            goto exit;
    }

    LOG(TEST, "RHI_SESSION_SEND completed\n");

exit:
    val_realm_return_to_host();
}
