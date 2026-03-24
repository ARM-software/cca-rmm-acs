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
#include "rhi_session_receive.h"

#define RECEIVE_DATA_PATTERN 0xabcdabcdU
#define RECEIVE_DATA_LENGTH 64U
#define RHI_INVALID_SESSION_ID 0xFFFFFFFF

static void build_expected(uint8_t *buffer)
{
    uint32_t idx;
    uint32_t pattern = RECEIVE_DATA_PATTERN;

    for (idx = 0; idx < RECEIVE_DATA_LENGTH; idx++)
        buffer[idx] = ((uint8_t *)&pattern)[idx % sizeof(pattern)];
}

static uint64_t g_session_id;

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

static uint64_t session_receive(uint64_t sess_id, uint64_t buffer_ipa,
                                uint64_t buffer_size, uint64_t offset,
                                uint64_t expected_status, uint64_t expected_state,
                                uint64_t expected_len)
{
    uint64_t ret;
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t rhi_call = {0};

    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_SESSION_RECEIVE;
    rhi_call.gprs[1] = sess_id;
    rhi_call.gprs[2] = buffer_ipa;
    rhi_call.gprs[3] = buffer_size;
    rhi_call.gprs[4] = offset;
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
            LOG(ERROR, "RHI_SESSION_RECEIVE failed: %lx\n", rhi_call.gprs[0]);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            return VAL_ERROR;
        }
        return VAL_SUCCESS;
    }

    if (rhi_call.gprs[0] != RHI_SESS_SUCCESS ||
        rhi_call.gprs[2] != expected_state ||
        rhi_call.gprs[3] != expected_len)
    {
        LOG(ERROR, "RHI_SESSION_RECEIVE failed: %lx state %lx len %lx\n",
            rhi_call.gprs[0], rhi_call.gprs[2], rhi_call.gprs[3]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
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
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        return VAL_ERROR;
    }

    if (rhi_call.gprs[0] != RHI_SESS_SUCCESS ||
        rhi_call.gprs[2] != expected_state)
    {
        LOG(ERROR, "RHI_SESSION_CLOSE failed: %lx state %lx\n",
                                                rhi_call.gprs[0], rhi_call.gprs[2]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void rhi_session_receive_realm(void)
{
    uint64_t ret;
    uint64_t buffer_ipa;
    uint64_t buffer_ipa_unaligned;
    uint8_t expected[RECEIVE_DATA_LENGTH];
    uint64_t i;
    const uint64_t session_id_current = ~0ULL;

    buffer_ipa = (uint64_t)(val_get_shared_region_base() + TEST_USE_OFFSET1);
    buffer_ipa_unaligned = buffer_ipa + 8;
    val_memset((void *)buffer_ipa, 0, PAGE_SIZE);
    val_memset(expected, 0, sizeof(expected));
    build_expected(expected);

    const struct session_step steps[] = {
        {ACTION_OPEN, 0x0, RHI_CONN_MODE_BLOCKING,
            0, 0, 0, RHI_SESS_SUCCESS, RHI_HSS_CONNECTION_ESTABLISHED, 0, false},
        {ACTION_RECEIVE, session_id_current, 0,
            0, 0, 0, RHI_SESS_SUCCESS, RHI_HSS_BUFFER_SIZE_DETERMINED,
            RECEIVE_DATA_LENGTH, false},
        {ACTION_RECEIVE, session_id_current, 0,
            buffer_ipa, 0, 0, RHI_SESS_SUCCESS, RHI_HSS_IO_COMPLETE, 0, false},
        {ACTION_RECEIVE, session_id_current, 0,
            buffer_ipa, PAGE_SIZE, 0, RHI_SESS_SUCCESS, RHI_HSS_IO_COMPLETE,
            RECEIVE_DATA_LENGTH, true},
        {ACTION_CLOSE, session_id_current, 0,
            0, 0, 0, RHI_SESS_SUCCESS, RHI_HSS_SESSION_UNCONNECTED, 0, false},
        {ACTION_RECEIVE, RHI_INVALID_SESSION_ID, 0,
            buffer_ipa, PAGE_SIZE, 0, RHI_SESS_INVALID_SESSION_ID, 0, 0, false},
        {ACTION_OPEN, 0x0, RHI_CONN_MODE_NON_BLOCKING,
            0, 0, 0, RHI_SESS_SUCCESS, RHI_HSS_CONNECTION_IN_PROGRESS, 0, false},
        {ACTION_RECEIVE, session_id_current, 0,
            buffer_ipa, PAGE_SIZE, 0, RHI_SESS_INVALID_STATE_FOR_OPERATION, 0, 0, false},
        {ACTION_OPEN, session_id_current, RHI_CONN_MODE_NON_BLOCKING,
            0, 0, 0, RHI_SESS_SUCCESS, RHI_HSS_CONNECTION_ESTABLISHED, 0, false},
        {ACTION_RECEIVE, session_id_current, 0,
            buffer_ipa_unaligned, PAGE_SIZE, 0, RHI_SESS_ACCESS_FAILED, 0, 0, false},
        {ACTION_RECEIVE, session_id_current, 0,
            buffer_ipa, PAGE_SIZE, 0, RHI_SESS_SUCCESS, RHI_HSS_IO_IN_PROGRESS, 0, false},
        {ACTION_RECEIVE, session_id_current, 0,
            buffer_ipa, PAGE_SIZE, 0, RHI_SESS_SUCCESS, RHI_HSS_IO_COMPLETE,
            RECEIVE_DATA_LENGTH, true},
        {ACTION_CLOSE, session_id_current, 0,
            0, 0, 0, RHI_SESS_SUCCESS, RHI_HSS_CONNECTION_CLOSE_IN_PROGRESS, 0, false},
        {ACTION_CLOSE, session_id_current, 0,
            0, 0, 0, RHI_SESS_SUCCESS, RHI_HSS_SESSION_UNCONNECTED, 0, false}
    };

    for (i = 0; i < (sizeof(steps) / sizeof(steps[0])); i++)
    {
        uint64_t sess_id = steps[i].sess_id;
        uint64_t recv_ipa = steps[i].buffer_ipa;

        if (sess_id == session_id_current)
            sess_id = g_session_id;

        if (steps[i].action == ACTION_OPEN) {
            ret = session_open(sess_id, steps[i].conn_type,
                               steps[i].expected_state);
        } else if (steps[i].action == ACTION_RECEIVE) {
            ret = session_receive(sess_id, recv_ipa,
                                  steps[i].buffer_size, steps[i].offset,
                                  steps[i].expected_status, steps[i].expected_state,
                                  steps[i].expected_len);
        } else {
            ret = session_close(sess_id, steps[i].expected_state);
        }

        if (ret)
            goto exit;

        if (steps[i].validate_data &&
            val_memcmp((void *)buffer_ipa, expected, RECEIVE_DATA_LENGTH))
        {
            LOG(ERROR, "RHI_SESSION_RECEIVE data mismatch\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
            goto exit;
        }
    }

    LOG(TEST, "RHI_SESSION_RECEIVE completed\n");

exit:
    val_realm_return_to_host();
}
