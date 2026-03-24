/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_rmi.h"
#include "val_libc.h"
#include "val.h"
#include "val_rhi.h"
#include "val_host_realm.h"
#include "val_host_da.h"
#include "val_host_rhi.h"
#include "val_host_alloc.h"

typedef struct rhi_session {
    uint64_t session_id;
    uint64_t proto_state;
    bool id_in_use;
    uint64_t conn_type;
} rhi_session_ts;

typedef struct rhi_session_io_state {
    uint64_t proto_state;
    uint64_t io_len;
} rhi_session_io_state_ts;

rhi_session_ts g_rhi_sess;
static rhi_session_io_state_ts g_rhi_send_state;
static rhi_session_io_state_ts g_rhi_recv_state;

static uint64_t g_da_next_handle;
static uint64_t g_fal_offset;

/**
 * @brief Allocate a new device assignment handle.
 *
 * @return Returns a monotonically increasing handle value.
 **/
static uint64_t val_da_alloc_handle(void)
{
    static uint64_t handle = 1;
    return handle++;
}

/**
 * @brief Handle RHI session open command from the Realm.
 *
 * Validates the session state and connection type, updates session tracking
 * state, and returns the session identifiers back to the Realm.
 *
 * @param  sess_id       - Session identifier provided by the Realm
 * @param  conn_type     - Requested connection mode
 * @param  realm         - Realm structure
 * @return Returns RHI session status code
 **/
static uint64_t val_host_rhi_session_open(uint64_t sess_id,
                                          uint64_t conn_type,
                                          val_host_realm_ts *realm)
{
    val_host_rec_enter_ts *rec_enter = NULL;

    rec_enter = &(((val_host_rec_run_ts *)realm->run[0])->enter);

    LOG(ALWAYS, "session_open: id: %lx conn type: %lx\n", sess_id, conn_type);

    /* Validate connection type support and value */
    if (!(conn_type & RHI_CONN_MODE_SUPPORTED_MASK))
        return RHI_SESS_CONNECTION_TYPE_NOT_SUPPORTED;

    /* Reject invalid mixed values (both bits set) */
    if ((conn_type & RHI_CONN_MODE_SUPPORTED_MASK) == RHI_CONN_MODE_SUPPORTED_MASK)
        return RHI_SESS_CONNECTION_TYPE_NOT_SUPPORTED;

    /* -------- BLOCKING -------- */
    if (conn_type == RHI_CONN_MODE_BLOCKING) {

        /* Invalid unless state is UNCONNECTED */
        if (g_rhi_sess.proto_state != RHI_HSS_SESSION_UNCONNECTED)
            return RHI_SESS_INVALID_STATE_FOR_OPERATION;

        /* Initial call rule: SessionID must be 0 */
        if (sess_id != 0 || g_rhi_sess.id_in_use)
            return RHI_SESS_INVALID_SESSION_ID;

        /* Success */
        g_rhi_sess.session_id  = pal_rhi_alloc_session_id();
        g_rhi_sess.id_in_use   = true;
        g_rhi_sess.proto_state = RHI_HSS_CONNECTION_ESTABLISHED;
        g_rhi_sess.conn_type = conn_type;
        g_rhi_send_state.proto_state = RHI_HSS_CONNECTION_ESTABLISHED;
        g_rhi_send_state.io_len = 0;
        g_rhi_recv_state.proto_state = RHI_HSS_CONNECTION_ESTABLISHED;
        g_rhi_recv_state.io_len = 0;

        rec_enter->gprs[1] = g_rhi_sess.session_id;
        rec_enter->gprs[2] = g_rhi_sess.proto_state;

        return RHI_SESS_SUCCESS;
    }

    /* -------- NON-BLOCKING -------- */
    if (conn_type == RHI_CONN_MODE_NON_BLOCKING) {

        /* Initial call: state UNCONNECTED */
        if (g_rhi_sess.proto_state == RHI_HSS_SESSION_UNCONNECTED) {

            /* Spec return condition: "0 not passed on initial call" */
            if (sess_id != 0 || g_rhi_sess.id_in_use)
                return RHI_SESS_INVALID_SESSION_ID;

            /* Return immediately in progress */
            g_rhi_sess.session_id  = pal_rhi_alloc_session_id();
            g_rhi_sess.id_in_use   = true;
            g_rhi_sess.proto_state = RHI_HSS_CONNECTION_IN_PROGRESS;
            g_rhi_sess.conn_type = conn_type;
            g_rhi_send_state.proto_state = RHI_HSS_CONNECTION_ESTABLISHED;
            g_rhi_send_state.io_len = 0;
            g_rhi_recv_state.proto_state = RHI_HSS_CONNECTION_ESTABLISHED;
            g_rhi_recv_state.io_len = 0;

            rec_enter->gprs[1] = g_rhi_sess.session_id;
            rec_enter->gprs[2] = g_rhi_sess.proto_state;

            return RHI_SESS_SUCCESS;
        }

        /* Retry calls while IN_PROGRESS for NON-BLOCKING mode */
        if (g_rhi_sess.proto_state == RHI_HSS_CONNECTION_IN_PROGRESS) {

            /* Spec: subsequent calls must pass SessionID returned from first call */
            if (!g_rhi_sess.id_in_use || sess_id == 0 || sess_id != g_rhi_sess.session_id)
                return RHI_SESS_INVALID_SESSION_ID;

            g_rhi_sess.proto_state = RHI_HSS_CONNECTION_ESTABLISHED;

            rec_enter->gprs[1] = g_rhi_sess.session_id;
            rec_enter->gprs[2] = g_rhi_sess.proto_state;

            return RHI_SESS_SUCCESS;
        }

        /* If state is ESTABLISHED (or anything else), OPEN is invalid by spec */
        return RHI_SESS_INVALID_STATE_FOR_OPERATION;
    }

    /* Should not reach here */
    return RHI_SESS_CONNECTION_TYPE_NOT_SUPPORTED;
}

/**
 * @brief Handle RHI session close command from the Realm.
 *
 * Validates the session identifier and protocol state, updates session
 * tracking state, and returns status to the Realm.
 *
 * @param  sess_id       - Session identifier provided by the Realm
 * @param  realm         - Realm structure
 * @return Returns RHI session status code
 **/
static uint64_t val_host_rhi_session_close(uint64_t sess_id, val_host_realm_ts *realm)
{
    val_host_rec_enter_ts *rec_enter = NULL;

    rec_enter = &(((val_host_rec_run_ts *)realm->run[0])->enter);

    LOG(ALWAYS, "session_close: id: %lx conn type: %lx\n", sess_id, g_rhi_sess.conn_type);
    LOG(ALWAYS, "session_close: state: %lx\n", g_rhi_sess.proto_state);
    /* Unknown SessionID parameter */
    if (!g_rhi_sess.id_in_use || sess_id == 0 || sess_id != g_rhi_sess.session_id)
        return RHI_SESS_INVALID_SESSION_ID;

    /* INVALID_STATE_FOR_OPERATION: state is UNCONNECTED */
    if (g_rhi_sess.proto_state == RHI_HSS_SESSION_UNCONNECTED)
        return RHI_SESS_INVALID_STATE_FOR_OPERATION;

    /* Decide behavior from session's connection mode (stored at OPEN) */
    if (g_rhi_sess.conn_type == RHI_CONN_MODE_BLOCKING) {

        /* BLOCKING close: only meaningful when established */
        if (g_rhi_sess.proto_state != RHI_HSS_CONNECTION_ESTABLISHED)
            return RHI_SESS_INVALID_STATE_FOR_OPERATION;

        /* Success -> connection_unconnected */
        g_rhi_sess.proto_state = RHI_HSS_SESSION_UNCONNECTED;

        rec_enter->gprs[1] = g_rhi_sess.session_id;
        rec_enter->gprs[2] = g_rhi_sess.proto_state;

        /* Success -> Reset the structure for future use */
        g_rhi_sess.id_in_use   = false;
        g_rhi_sess.session_id  = 0;
        g_rhi_sess.conn_type   = 0;
        g_rhi_send_state.proto_state = RHI_HSS_SESSION_UNCONNECTED;
        g_rhi_send_state.io_len = 0;
        g_rhi_recv_state.proto_state = RHI_HSS_SESSION_UNCONNECTED;
        g_rhi_recv_state.io_len = 0;

        return RHI_SESS_SUCCESS;
    }

    /* NON-BLOCKING close */
    if (g_rhi_sess.conn_type == RHI_CONN_MODE_NON_BLOCKING) {

        /* Initial close call from ESTABLISHED: start async close, return immediately */
        if (g_rhi_sess.proto_state == RHI_HSS_CONNECTION_ESTABLISHED) {

            g_rhi_sess.proto_state = RHI_HSS_CONNECTION_CLOSE_IN_PROGRESS;

            rec_enter->gprs[1] = g_rhi_sess.session_id;
            rec_enter->gprs[2] = g_rhi_sess.proto_state;

            return RHI_SESS_SUCCESS;
        }

        /* Subsequent calls while CLOSE_IN_PROGRESS: poll until done or error */
        if (g_rhi_sess.proto_state == RHI_HSS_CONNECTION_CLOSE_IN_PROGRESS) {

            /* Success -> UNCONNECTED */
            g_rhi_sess.proto_state = RHI_HSS_SESSION_UNCONNECTED;
            rec_enter->gprs[1] = g_rhi_sess.session_id;
            rec_enter->gprs[2] = g_rhi_sess.proto_state;

            /* Success -> Reset the structure for future use */
            g_rhi_sess.id_in_use   = false;
            g_rhi_sess.session_id  = 0;
            g_rhi_sess.conn_type   = 0;
            g_rhi_send_state.proto_state = RHI_HSS_SESSION_UNCONNECTED;
            g_rhi_send_state.io_len = 0;
            g_rhi_recv_state.proto_state = RHI_HSS_SESSION_UNCONNECTED;
            g_rhi_recv_state.io_len = 0;

            return RHI_SESS_SUCCESS;
        }

        /* Any other state is not valid for CLOSE */
        return RHI_SESS_INVALID_STATE_FOR_OPERATION;
    }

    /* If conn_type wasn't recorded properly */
    return RHI_SESS_INVALID_STATE_FOR_OPERATION;
}

/**
 * @brief Handle RHI session send command from the Realm.
 *
 * Validates session state, copies payload from shared buffer, and updates
 * I/O state for blocking or non-blocking modes.
 *
 * @param  sess_id       - Session identifier provided by the Realm
 * @param  buffer_ipa    - IPA of the payload buffer
 * @param  length        - Length of the payload to send
 * @param  offset        - Offset into the payload buffer
 * @param  realm         - Realm structure
 * @return Returns RHI session status code
 **/
static uint64_t val_host_rhi_session_send(uint64_t sess_id,
                                          uint64_t buffer_ipa,
                                          uint64_t length,
                                          uint64_t offset,
                                          val_host_realm_ts *realm)
{
    val_host_rec_enter_ts *rec_enter = NULL;
    uint8_t *local_copy = NULL;
    uint8_t *expected = NULL;
    uint64_t idx;
    uint32_t pattern = 0xabcdabcdU;

    rec_enter = &(((val_host_rec_run_ts *)realm->run[0])->enter);

    if (!g_rhi_sess.id_in_use || sess_id == 0 || sess_id != g_rhi_sess.session_id)
        return RHI_SESS_INVALID_SESSION_ID;

    if (g_rhi_sess.proto_state == RHI_HSS_SESSION_UNCONNECTED ||
        g_rhi_sess.proto_state == RHI_HSS_CONNECTION_IN_PROGRESS ||
        g_rhi_sess.proto_state == RHI_HSS_CONNECTION_CLOSE_IN_PROGRESS)
        return RHI_SESS_INVALID_STATE_FOR_OPERATION;

    if (buffer_ipa == 0 || !ADDR_IS_ALIGNED(buffer_ipa, PAGE_SIZE))
        return RHI_SESS_ACCESS_FAILED;

    buffer_ipa = (uint64_t)(val_get_shared_region_base() + TEST_USE_OFFSET1);

    if (g_rhi_sess.conn_type == RHI_CONN_MODE_BLOCKING) {
        if (g_rhi_send_state.proto_state != RHI_HSS_CONNECTION_ESTABLISHED &&
            g_rhi_send_state.proto_state != RHI_HSS_IO_COMPLETE)
            return RHI_SESS_INVALID_STATE_FOR_OPERATION;

        local_copy = val_host_mem_alloc(PAGE_SIZE, length);
        expected = val_host_mem_alloc(PAGE_SIZE, length);
        if (!local_copy || !expected)
            return RHI_SESS_ACCESS_FAILED;

        val_memcpy(local_copy, (void *)(buffer_ipa + offset), length);
        for (idx = 0; idx < length; idx++) {
            expected[idx] = ((uint8_t *)&pattern)[idx % sizeof(pattern)];
        }

        if (val_memcmp(local_copy, expected, length))
        {
            rec_enter->gprs[1] = g_rhi_sess.session_id;
            rec_enter->gprs[2] = RHI_HSS_CONNECTION_ESTABLISHED;
            rec_enter->gprs[3] = 0;
            g_rhi_sess.proto_state = RHI_HSS_CONNECTION_ESTABLISHED;
            return RHI_SESS_PEER_NOT_AVAILABLE;
        }

        g_rhi_send_state.proto_state = RHI_HSS_IO_COMPLETE;
        g_rhi_send_state.io_len = length;
        rec_enter->gprs[1] = g_rhi_sess.session_id;
        rec_enter->gprs[2] = g_rhi_send_state.proto_state;
        rec_enter->gprs[3] = length;
        return RHI_SESS_SUCCESS;
    }

    if (g_rhi_sess.conn_type == RHI_CONN_MODE_NON_BLOCKING) {
        if (g_rhi_send_state.proto_state == RHI_HSS_IO_IN_PROGRESS) {
            g_rhi_send_state.proto_state = RHI_HSS_IO_COMPLETE;
            rec_enter->gprs[1] = g_rhi_sess.session_id;
            rec_enter->gprs[2] = g_rhi_send_state.proto_state;
            rec_enter->gprs[3] = g_rhi_send_state.io_len;
            return RHI_SESS_SUCCESS;
        }

        if (g_rhi_send_state.proto_state != RHI_HSS_CONNECTION_ESTABLISHED &&
            g_rhi_send_state.proto_state != RHI_HSS_IO_COMPLETE)
            return RHI_SESS_INVALID_STATE_FOR_OPERATION;

        local_copy = val_host_mem_alloc(PAGE_SIZE, length);
        expected = val_host_mem_alloc(PAGE_SIZE, length);
        if (!local_copy || !expected)
            return RHI_SESS_ACCESS_FAILED;

        val_memcpy(local_copy, (void *)(buffer_ipa + offset), length);
        for (idx = 0; idx < length; idx++) {
            expected[idx] = ((uint8_t *)&pattern)[idx % sizeof(pattern)];
        }

        if (val_memcmp(local_copy, expected, length))
        {
            rec_enter->gprs[1] = g_rhi_sess.session_id;
            rec_enter->gprs[2] = RHI_HSS_CONNECTION_ESTABLISHED;
            rec_enter->gprs[3] = 0;
            g_rhi_sess.proto_state = RHI_HSS_CONNECTION_ESTABLISHED;
            return RHI_SESS_PEER_NOT_AVAILABLE;
        }

        g_rhi_send_state.proto_state = RHI_HSS_IO_IN_PROGRESS;
        g_rhi_send_state.io_len = length;
        rec_enter->gprs[1] = g_rhi_sess.session_id;
        rec_enter->gprs[2] = g_rhi_send_state.proto_state;
        rec_enter->gprs[3] = 0;
        return RHI_SESS_SUCCESS;
    }

    return RHI_SESS_INVALID_STATE_FOR_OPERATION;
}

/**
 * @brief Handle RHI session receive command from the Realm.
 *
 * Validates session state, writes payload into the shared buffer, and updates
 * I/O state for blocking or non-blocking modes.
 *
 * @param  sess_id       - Session identifier provided by the Realm
 * @param  buffer_ipa    - IPA of the destination buffer
 * @param  buffer_size   - Size of the destination buffer
 * @param  offset        - Offset into the destination buffer
 * @param  realm         - Realm structure
 * @return Returns RHI session status code
 **/
static uint64_t val_host_rhi_session_receive(uint64_t sess_id,
                                             uint64_t buffer_ipa,
                                             uint64_t buffer_size,
                                             uint64_t offset,
                                             val_host_realm_ts *realm)
{
    val_host_rec_enter_ts *rec_enter = NULL;
    uint8_t *dest;
    uint64_t idx;
    uint32_t pattern = 0xabcdabcdU;
    uint64_t payload_len = 64;

    rec_enter = &(((val_host_rec_run_ts *)realm->run[0])->enter);

    if (!g_rhi_sess.id_in_use || sess_id == 0 || sess_id != g_rhi_sess.session_id)
        return RHI_SESS_INVALID_SESSION_ID;

    if (g_rhi_sess.proto_state == RHI_HSS_SESSION_UNCONNECTED ||
        g_rhi_sess.proto_state == RHI_HSS_CONNECTION_IN_PROGRESS ||
        g_rhi_sess.proto_state == RHI_HSS_CONNECTION_CLOSE_IN_PROGRESS)
        return RHI_SESS_INVALID_STATE_FOR_OPERATION;

    if (buffer_ipa == 0 && buffer_size == 0)
    {
        rec_enter->gprs[1] = g_rhi_sess.session_id;
        rec_enter->gprs[2] = RHI_HSS_BUFFER_SIZE_DETERMINED;
        rec_enter->gprs[3] = payload_len;
        return RHI_SESS_SUCCESS;
    }

    if (buffer_ipa == 0 || !ADDR_IS_ALIGNED(buffer_ipa, PAGE_SIZE))
        return RHI_SESS_ACCESS_FAILED;

    buffer_ipa = (uint64_t)(val_get_shared_region_base() + TEST_USE_OFFSET1);

    if (buffer_size == 0)
    {
        rec_enter->gprs[1] = g_rhi_sess.session_id;
        rec_enter->gprs[2] = RHI_HSS_IO_COMPLETE;
        rec_enter->gprs[3] = 0;
        return RHI_SESS_SUCCESS;
    }

    if (offset >= buffer_size || payload_len > (buffer_size - offset))
        return RHI_SESS_ACCESS_FAILED;

    if (g_rhi_sess.conn_type == RHI_CONN_MODE_BLOCKING) {
        if (g_rhi_recv_state.proto_state != RHI_HSS_CONNECTION_ESTABLISHED &&
            g_rhi_recv_state.proto_state != RHI_HSS_IO_COMPLETE)
            return RHI_SESS_INVALID_STATE_FOR_OPERATION;

        dest = (uint8_t *)(buffer_ipa + offset);
        for (idx = 0; idx < payload_len; idx++) {
            dest[idx] = ((uint8_t *)&pattern)[idx % sizeof(pattern)];
        }

        g_rhi_recv_state.proto_state = RHI_HSS_IO_COMPLETE;
        g_rhi_recv_state.io_len = payload_len;
        rec_enter->gprs[1] = g_rhi_sess.session_id;
        rec_enter->gprs[2] = g_rhi_recv_state.proto_state;
        rec_enter->gprs[3] = payload_len;
        return RHI_SESS_SUCCESS;
    }

    if (g_rhi_sess.conn_type == RHI_CONN_MODE_NON_BLOCKING) {
        if (g_rhi_recv_state.proto_state == RHI_HSS_IO_IN_PROGRESS) {
            g_rhi_recv_state.proto_state = RHI_HSS_IO_COMPLETE;
            rec_enter->gprs[1] = g_rhi_sess.session_id;
            rec_enter->gprs[2] = g_rhi_recv_state.proto_state;
            rec_enter->gprs[3] = g_rhi_recv_state.io_len;
            return RHI_SESS_SUCCESS;
        }

        if (g_rhi_recv_state.proto_state != RHI_HSS_CONNECTION_ESTABLISHED &&
            g_rhi_recv_state.proto_state != RHI_HSS_IO_COMPLETE)
            return RHI_SESS_INVALID_STATE_FOR_OPERATION;

        dest = (uint8_t *)(buffer_ipa + offset);
        for (idx = 0; idx < payload_len; idx++) {
            dest[idx] = ((uint8_t *)&pattern)[idx % sizeof(pattern)];
        }

        g_rhi_recv_state.proto_state = RHI_HSS_IO_IN_PROGRESS;
        g_rhi_recv_state.io_len = payload_len;
        rec_enter->gprs[1] = g_rhi_sess.session_id;
        rec_enter->gprs[2] = g_rhi_recv_state.proto_state;
        rec_enter->gprs[3] = 0;
        return RHI_SESS_SUCCESS;
    }

    return RHI_SESS_INVALID_STATE_FOR_OPERATION;
}

/**
 * @brief Handle RHI FAL read command from the Realm.
 *
 * Populates a synthetic log buffer and returns chunks of data to the Realm.
 *
 * @param  buffer_ipa    - IPA of the output buffer
 * @param  buffer_size   - Size of the output buffer
 * @param  realm         - Realm structure
 * @return Returns RHI FAL status code
 **/
static uint64_t val_host_rhi_fal_read(uint64_t buffer_ipa,
                                      uint64_t buffer_size,
                                      val_host_realm_ts *realm)
{
    val_host_rec_enter_ts *rec_enter = NULL;
    static uint8_t fal_log[RHI_FAL_LOG_SIZE];
    static bool fal_init;
    uint64_t remaining;
    uint64_t copy_len;
    uint64_t idx;
    uint32_t pattern = 0xabcdabcdU;

    rec_enter = &(((val_host_rec_run_ts *)realm->run[0])->enter);

    if (buffer_ipa == 0 || !ADDR_IS_ALIGNED(buffer_ipa, PAGE_SIZE))
        return RHI_FAL_ACCESS_FAILED;

    buffer_ipa = (uint64_t)(val_get_shared_region_base() + TEST_USE_OFFSET1);

    if (!fal_init)
    {
        for (idx = 0; idx < RHI_FAL_LOG_SIZE; idx++)
        {
            fal_log[idx] = ((uint8_t *)&pattern)[idx % sizeof(pattern)];
        }
        fal_init = true;
    }

    if (g_fal_offset >= RHI_FAL_LOG_SIZE || buffer_size == 0)
    {
        rec_enter->gprs[1] = 0;
        rec_enter->gprs[2] = 0;
        return RHI_FAL_SUCCESS;
    }

    remaining = RHI_FAL_LOG_SIZE - g_fal_offset;
    copy_len = MIN(buffer_size, remaining);
    val_memcpy((void *)buffer_ipa, fal_log + g_fal_offset, copy_len);
    g_fal_offset += copy_len;
    remaining = RHI_FAL_LOG_SIZE - g_fal_offset;

    rec_enter->gprs[1] = copy_len;
    rec_enter->gprs[2] = remaining;
    return RHI_FAL_SUCCESS;
}

/**
 * @brief Handle RHI DA VDEV_CONTINUE command from the Realm.
 *
 * Validates the handle, locates the target VDEV, and performs device
 * communication on behalf of the Realm.
 *
 * @param  realm         - Realm structure
 * @param  pdev_obj      - Physical device object
 * @param  vdev_obj      - Virtual device object(s)
 * @return Returns RHI DA status code
 **/
uint64_t val_rhi_da_vdev_continue(val_host_realm_ts *realm,
                                  val_host_pdev_ts *pdev_obj, val_host_vdev_ts *vdev_obj)
{
    val_host_rec_exit_ts *rec_exit = NULL;
    val_host_vdev_ts *target_vdev = vdev_obj;
    uint64_t handle;
    uint64_t vdev_id;

    rec_exit = &(((val_host_rec_run_ts *)realm->run[0])->exit);

    vdev_id = rec_exit->gprs[1];
    handle = rec_exit->gprs[2];

    if (g_da_next_handle != handle)
        return RHI_DA_ERROR_INVALID_VDEV_ID;
    if (vdev_obj->vdev_id != vdev_id)
    {
        if (g_da_vdev_count > 1 && vdev_obj[1].vdev_id == vdev_id)
            target_vdev = &vdev_obj[1];
        else
            return RHI_DA_ERROR_INVALID_VDEV_ID;
    }

    if (val_host_dev_communicate(realm, pdev_obj, target_vdev, RMI_VDEV_LOCKED))
    {
        LOG(ERROR, "VDEV communicate failed\n");
        return RHI_DA_ERROR_DEVICE;
    }

    LOG(TEST, "%s completed\n", __func__);
    return RHI_DA_SUCCESS;
}

/**
 * @brief Handle RHI DA VDEV_GET_MEASUREMENTS command from the Realm.
 *
 * Fetches measurement data from the VDEV and returns an asynchronous handle
 * to the Realm.
 *
 * @param  realm         - Realm structure
 * @param  vdev_obj      - Virtual device object
 * @return Returns RHI DA status code
 **/
uint64_t val_rhi_da_vdev_get_measurements(val_host_realm_ts *realm,
                                          val_host_vdev_ts *vdev_obj)
{
    val_host_rec_enter_ts *rec_enter = NULL;
    val_host_rec_exit_ts *rec_exit = NULL;
    uint64_t vdev_id;
    uint64_t params_ipa;
    val_smc_param_ts args;
    val_host_vdev_measure_params_ts *vdev_measure_params;

    rec_enter = &(((val_host_rec_run_ts *)realm->run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm->run[0])->exit);

    vdev_id = rec_exit->gprs[1];
    params_ipa = rec_exit->gprs[2];
    vdev_measure_params = (val_host_vdev_measure_params_ts *)(val_get_shared_region_base() +
                                                              TEST_USE_OFFSET1);

    if (vdev_obj->vdev_id != vdev_id)
        return RHI_DA_ERROR_INVALID_VDEV_ID;
    if (!params_ipa)
        return RHI_DA_ERROR_ACCESS_FAILED;

    args = val_host_rmi_vdev_get_measurements(realm->rd, vdev_obj->pdev,
                                              vdev_obj->vdev, (uint64_t)vdev_measure_params);
    if (args.x0)
    {
        LOG(ERROR, "VDEV get measurements failed: %lx\n", args.x0);
        return RHI_DA_ERROR_DEVICE;
    }

    rec_enter->gprs[1] = val_da_alloc_handle();
    g_da_next_handle = rec_enter->gprs[1];

    LOG(TEST, "%s completed\n", __func__);
    return RHI_DA_ERROR_INCOMPLETE;
}

/**
 * @brief Handle RHI DA OBJECT_SIZE command from the Realm.
 *
 * Determines the size of the requested DA object for the specified VDEV.
 *
 * @param  realm         - Realm structure
 * @param  pdev_obj      - Physical device object
 * @param  vdev_obj      - Virtual device object
 * @return Returns RHI DA status code
 **/
uint64_t val_rhi_da_object_size(val_host_realm_ts *realm,
                                val_host_pdev_ts *pdev_obj,
                                val_host_vdev_ts *vdev_obj)
{
    val_host_rec_enter_ts *rec_enter = NULL;
    val_host_rec_exit_ts *rec_exit = NULL;
    uint64_t vdev_id;
    uint64_t obj_type;
    uint64_t size;

    rec_enter = &(((val_host_rec_run_ts *)realm->run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm->run[0])->exit);

    vdev_id = rec_exit->gprs[1];
    obj_type = rec_exit->gprs[2];

    if (vdev_obj->vdev_id != vdev_id)
        return RHI_DA_ERROR_INVALID_VDEV_ID;

    switch (obj_type) {
    case RHI_DA_DEV_VCA:
        size = pdev_obj->vca_len;
        break;
    case RHI_DA_DEV_CERTIFICATE:
        size = pdev_obj->cert_chain_len;
        break;
    case RHI_DA_DEV_MEASUREMENTS:
        size = vdev_obj->meas_len;
        break;
    case RHI_DA_DEV_INTERFACE_REPORT:
        size = vdev_obj->ifc_report_len;
        break;
    case RHI_DA_DEV_EXTENSION_EVIDENCE:
        return RHI_DA_ERROR_OBJECT_UNSUPPORTED;
    default:
        return RHI_DA_ERROR_INVALID_OBJECT;
    }

    if (size == 0)
        return RHI_DA_ERROR_DATA_NOT_AVAILABLE;

    rec_enter->gprs[1] = size;
    LOG(TEST, "%s completed\n", __func__);
    return RHI_DA_SUCCESS;
}

/**
 * @brief Handle RHI DA OBJECT_READ command from the Realm.
 *
 * Copies the requested DA object into the shared buffer for the Realm.
 *
 * @param  realm         - Realm structure
 * @param  pdev_obj      - Physical device object
 * @param  vdev_obj      - Virtual device object
 * @return Returns RHI DA status code
 **/
uint64_t val_rhi_da_object_read(val_host_realm_ts *realm,
                                val_host_pdev_ts *pdev_obj,
                                val_host_vdev_ts *vdev_obj)
{
    val_host_rec_enter_ts *rec_enter = NULL;
    val_host_rec_exit_ts *rec_exit = NULL;
    const uint8_t *src = NULL;
    uint64_t vdev_id;
    uint64_t obj_type;
    uint64_t buffer_ipa;
    uint64_t max_len;
    uint64_t offset;
    uint64_t size;

    rec_enter = &(((val_host_rec_run_ts *)realm->run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm->run[0])->exit);

    vdev_id = rec_exit->gprs[1];
    obj_type = rec_exit->gprs[2];
    buffer_ipa = rec_exit->gprs[3];
    max_len = rec_exit->gprs[4];
    offset = rec_exit->gprs[5];

    if (vdev_obj->vdev_id != vdev_id)
        return RHI_DA_ERROR_INVALID_VDEV_ID;
    if (buffer_ipa == 0)
        return RHI_DA_ERROR_ACCESS_FAILED;

    buffer_ipa = (uint64_t)(val_get_shared_region_base() + TEST_USE_OFFSET1);

    switch (obj_type) {
    case RHI_DA_DEV_VCA:
        src = pdev_obj->vca;
        size = pdev_obj->vca_len;
        break;
    case RHI_DA_DEV_CERTIFICATE:
        src = pdev_obj->cert_chain;
        size = pdev_obj->cert_chain_len;
        break;
    case RHI_DA_DEV_MEASUREMENTS:
        src = vdev_obj->meas;
        size = vdev_obj->meas_len;
        break;
    case RHI_DA_DEV_INTERFACE_REPORT:
        src = vdev_obj->ifc_report;
        size = vdev_obj->ifc_report_len;
        break;
    case RHI_DA_DEV_EXTENSION_EVIDENCE:
        return RHI_DA_ERROR_OBJECT_UNSUPPORTED;
    default:
        return RHI_DA_ERROR_INVALID_OBJECT;
    }

    if (size == 0)
        return RHI_DA_ERROR_DATA_NOT_AVAILABLE;
    if (max_len == 0 || offset >= max_len || size > (max_len - offset))
        return RHI_DA_ERROR_INVALID_OFFSET;

    val_memcpy((void *)(buffer_ipa + offset), src, size);
    rec_enter->gprs[1] = size;
    LOG(TEST, "%s completed\n", __func__);
    return RHI_DA_SUCCESS;
}

/**
 * @brief Handle RHI DA VDEV_SET_TDI_STATE command from the Realm.
 *
 * Validates the requested TDI state, triggers a lock when requested, and
 * returns an asynchronous handle for completion.
 *
 * @param  realm         - Realm structure
 * @param  vdev_obj      - Virtual device object
 * @return Returns RHI DA status code
 **/
uint64_t val_rhi_da_vdev_set_tdi_state(val_host_realm_ts *realm,
                                      val_host_vdev_ts *vdev_obj)
{
    val_host_rec_enter_ts *rec_enter = NULL;
    val_host_rec_exit_ts *rec_exit = NULL;
    uint64_t vdev_id;
    uint64_t tdi_state;
    val_smc_param_ts args;

    rec_enter = &(((val_host_rec_run_ts *)realm->run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm->run[0])->exit);

    vdev_id = rec_exit->gprs[1];
    tdi_state = rec_exit->gprs[2];

    if (vdev_obj->vdev_id != vdev_id)
        return RHI_DA_ERROR_INVALID_VDEV_ID;

    /*
     * RHIDAVDevTDIState validation.
     * The skeleton implementation treats the state as a simple enable/disable
     * flag (0 or 1). Any other value is rejected.
     */
    if (tdi_state > RHI_DA_TDI_CONFIG_RUN)
        return RHI_DA_ERROR_INVALID_OBJECT;

    if (tdi_state == RHI_DA_TDI_CONFIG_LOCKED)
    {
        args = val_host_rmi_vdev_lock(realm->rd, vdev_obj->pdev, vdev_obj->vdev);
        if (args.x0)
        {
            LOG(ERROR, "\tVDEV_LOCK failed, ret=%x\n", args.x0);
            return VAL_ERROR;
        }
    }

    /*
     * ABI: RHI_DA_VDEV_SET_TDI_STATE may require the device communication loop.
     * Model this by returning INCOMPLETE with a handle and expecting the Realm
     * to invoke RHI_DA_VDEV_CONTINUE.
     */
    rec_enter->gprs[1] = val_da_alloc_handle();
    g_da_next_handle = rec_enter->gprs[1];

    LOG(TEST, "%s accepted (vdev_id=%lx, tdi_state=%lx), follow continue loop\n",
        __func__, vdev_id, tdi_state);
    return RHI_DA_ERROR_INCOMPLETE;
}

/**
 * @brief Handle RHI DA VDEV_ABORT command from the Realm.
 *
 * Aborts an in-progress device operation for the specified VDEV.
 *
 * @param  realm         - Realm structure
 * @param  vdev_obj      - Virtual device object
 * @return Returns RHI DA status code
 **/
uint64_t val_rhi_da_vdev_abort(val_host_realm_ts *realm,
                               val_host_vdev_ts *vdev_obj)
{
    val_host_rec_exit_ts *rec_exit = NULL;
    uint64_t vdev_id;
    val_smc_param_ts args;

    rec_exit = &(((val_host_rec_run_ts *)realm->run[0])->exit);

    vdev_id = rec_exit->gprs[1];

    if (vdev_obj->vdev_id != vdev_id)
        return RHI_DA_ERROR_INVALID_VDEV_ID;

    args = val_host_rmi_vdev_abort(vdev_obj->vdev);
    if (args.x0)
    {
        LOG(ERROR, "VDEV abort failed: %lx\n", args.x0);
        return RHI_DA_ABORTED_OPERATION_HAD_COMPLETED;
    }

    return RHI_DA_SUCCESS;
}

/**
 * @brief Handle RHI DA VDEV_P2P_UNBIND command from the Realm.
 *
 * Validates the two VDEV identifiers and returns an asynchronous handle for
 * the Realm to continue the operation.
 *
 * @param  realm         - Realm structure
 * @param  pdev_obj      - Physical device object
 * @param  vdev_obj      - Virtual device object array
 * @return Returns RHI DA status code
 **/
uint64_t val_rhi_da_vdev_p2p_unbind(val_host_realm_ts *realm,
                                    val_host_pdev_ts *pdev_obj,
                                    val_host_vdev_ts *vdev_obj)
{
    val_host_rec_enter_ts *rec_enter = NULL;
    val_host_rec_exit_ts *rec_exit = NULL;
    uint64_t vdev_id_1;
    uint64_t vdev_id_2;

    (void)pdev_obj;

    rec_enter = &(((val_host_rec_run_ts *)realm->run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm->run[0])->exit);

    vdev_id_1 = rec_exit->gprs[1];
    vdev_id_2 = rec_exit->gprs[2];

    if ((vdev_obj[0].vdev_id != vdev_id_1 && vdev_obj[1].vdev_id != vdev_id_1) ||
        (vdev_obj[0].vdev_id != vdev_id_2 && vdev_obj[1].vdev_id != vdev_id_2))
        return RHI_DA_ERROR_INVALID_VDEV_ID;

    rec_enter->gprs[1] = val_da_alloc_handle();
    g_da_next_handle = rec_enter->gprs[1];

    LOG(TEST, "%s accepted (vdev_id_1=%lx, vdev_id_2=%lx)\n",
        __func__, vdev_id_1, vdev_id_2);
    return RHI_DA_ERROR_INCOMPLETE;
}

/**
 * @brief Handle RHI DA VDEV_GET_INTERFACE_REPORT command from the Realm.
 *
 * Requests a VDEV interface report and returns an asynchronous handle.
 *
 * @param  realm         - Realm structure
 * @param  vdev_obj      - Virtual device object
 * @return Returns RHI DA status code
 **/
uint64_t val_rhi_da_vdev_get_interface_report(val_host_realm_ts *realm,
                                              val_host_vdev_ts *vdev_obj)
{
    val_host_rec_enter_ts *rec_enter = NULL;
    val_host_rec_exit_ts *rec_exit = NULL;
    uint64_t vdev_id;
    val_smc_param_ts args;

    rec_enter = &(((val_host_rec_run_ts *)realm->run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm->run[0])->exit);

    vdev_id = rec_exit->gprs[1];

    if (vdev_obj->vdev_id != vdev_id)
        return RHI_DA_ERROR_INVALID_VDEV_ID;

    args = val_host_rmi_vdev_get_interface_report(realm->rd, vdev_obj->pdev,
                                                           vdev_obj->vdev);
    if (args.x0)
    {
        LOG(ERROR, "VDEV interface report failed: %lx\n", args.x0);
        return RHI_DA_ERROR_BUSY;
    }

    rec_enter->gprs[1] = val_da_alloc_handle();
    g_da_next_handle = rec_enter->gprs[1];

    LOG(TEST, "%s completed\n", __func__);
    return RHI_DA_ERROR_INCOMPLETE;
}

/**
 * @brief RHI skeleton DA dispatcher for Realm Host Interface (RHI) commands.
 *
 * This function decodes the RHI SMCCC Function ID (FID) issued by the Realm
 * and dispatches it to the corresponding RHI command handler.
 *
 * @param  realm         - Realm structure
 * @param  pdev_obj      - Physical device object
 * @param  vdev_obj      - Virtual device object
 * @return Returns status code indicating success or failure of the dispatch
**/
uint64_t val_host_rhi_da_dispatch(val_host_realm_ts *realm,
                                  val_host_pdev_ts *pdev_obj, val_host_vdev_ts *vdev_obj)
{
    val_host_rec_enter_ts *rec_enter = NULL;
    val_host_rec_exit_ts *rec_exit = NULL;
    uint64_t fid;

    if (!realm)
        return VAL_ERROR;

    rec_enter = &(((val_host_rec_run_ts *)realm->run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm->run[0])->exit);

    fid = rec_exit->gprs[0];

    switch (fid) {
    /* Device Assignment */
    case RHI_DA_VERSION:
        rec_enter->gprs[0] = RHI_DA_VER;
        return VAL_SUCCESS;
    case RHI_DA_FEATURES:
        rec_enter->gprs[0] = RHI_DA_SUPPORTED_MASK;
        return VAL_SUCCESS;
    case RHI_DA_OBJECT_SIZE:
        rec_enter->gprs[0] = val_rhi_da_object_size(realm, pdev_obj, vdev_obj);
        return VAL_SUCCESS;
    case RHI_DA_OBJECT_READ:
        rec_enter->gprs[0] = val_rhi_da_object_read(realm, pdev_obj, vdev_obj);
        return VAL_SUCCESS;
    case RHI_DA_VDEV_CONTINUE:
        rec_enter->gprs[0] = val_rhi_da_vdev_continue(realm, pdev_obj, vdev_obj);
        return VAL_SUCCESS;
    case RHI_DA_VDEV_GET_MEASUREMENTS:
        rec_enter->gprs[0] = val_rhi_da_vdev_get_measurements(realm, vdev_obj);
        return VAL_SUCCESS;
    case RHI_DA_VDEV_GET_INTERFACE_REPORT:
        rec_enter->gprs[0] = val_rhi_da_vdev_get_interface_report(realm, vdev_obj);
        return VAL_SUCCESS;
    case RHI_DA_VDEV_SET_TDI_STATE:
        rec_enter->gprs[0] = val_rhi_da_vdev_set_tdi_state(realm, vdev_obj);
        return VAL_SUCCESS;
    case RHI_DA_VDEV_P2P_UNBIND:
        rec_enter->gprs[0] = val_rhi_da_vdev_p2p_unbind(realm, pdev_obj, vdev_obj);
        return VAL_SUCCESS;
    case RHI_DA_VDEV_ABORT:
        rec_enter->gprs[0] = val_rhi_da_vdev_abort(realm, vdev_obj);
        return VAL_SUCCESS;
    default:
        /* Unknown FID */
        LOG(ERROR, "RHI DA invalid fid: %lx\n", fid);
        return VAL_ERROR;
    }
}

/**
 * @brief RHI skeleton dispatcher for Realm Host Interface (RHI) commands.
 *
 * This function decodes the RHI SMCCC Function ID (FID) issued by the Realm
 * and dispatches it to the corresponding RHI command handler.
 *
 * @param  realm         - Realm structure
 * @return Returns status code indicating success or failure of the dispatch
**/
uint64_t val_host_rhi_dispatch(val_host_realm_ts *realm)
{
    val_host_rec_enter_ts *rec_enter = NULL;
    val_host_rec_exit_ts *rec_exit = NULL;
    uint64_t fid;

    if (!realm)
        return VAL_ERROR;

    rec_enter = &(((val_host_rec_run_ts *)realm->run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm->run[0])->exit);

    fid = rec_exit->gprs[0];

    switch (fid) {
    /* Host Session */
    case RHI_SESSION_VERSION:
        rec_enter->gprs[0] = RHI_SESSION_VER;
        return VAL_SUCCESS;
    case RHI_SESSION_FEATURES:
        rec_enter->gprs[0] = RHI_SESSION_SUPPORTED_MASK;
        rec_enter->gprs[1] = RHI_CONN_MODE_SUPPORTED_MASK;
        return VAL_SUCCESS;
    case RHI_SESSION_OPEN:
        rec_enter->gprs[0] = val_host_rhi_session_open(rec_exit->gprs[1], rec_exit->gprs[2], realm);
        return VAL_SUCCESS;
    case RHI_SESSION_CLOSE:
        rec_enter->gprs[0] = val_host_rhi_session_close(rec_exit->gprs[1], realm);
        return VAL_SUCCESS;
    case RHI_SESSION_SEND:
        rec_enter->gprs[0] = val_host_rhi_session_send(rec_exit->gprs[1],
                                                       rec_exit->gprs[2],
                                                       rec_exit->gprs[3],
                                                       rec_exit->gprs[4],
                                                       realm);
        return VAL_SUCCESS;
    case RHI_SESSION_RECEIVE:
        rec_enter->gprs[0] = val_host_rhi_session_receive(rec_exit->gprs[1],
                                                          rec_exit->gprs[2],
                                                          rec_exit->gprs[3],
                                                          rec_exit->gprs[4],
                                                          realm);
        return VAL_SUCCESS;

    /* Firmware Activity Log */
    case RHI_FAL_VERSION:
        rec_enter->gprs[0] = RHI_FAL_VER;
        return VAL_SUCCESS;
    case RHI_FAL_FEATURES:
        rec_enter->gprs[0] = RHI_FAL_SUPPORTED_MASK;
        return VAL_SUCCESS;
    case RHI_FAL_GET_SIZE:
        rec_enter->gprs[0] = RHI_FAL_LOG_SIZE;
        return VAL_SUCCESS;
    case RHI_FAL_READ:
        rec_enter->gprs[0] = val_host_rhi_fal_read(rec_exit->gprs[1],
                                                   rec_exit->gprs[2],
                                                   realm);
        return VAL_SUCCESS;


    /* Host Configuration */
    case RHI_HOSTCONF_VERSION:
        rec_enter->gprs[0] = RHI_HOSTCONF_VER;
        return VAL_SUCCESS;
    case RHI_HOSTCONF_FEATURES:
        rec_enter->gprs[0] = RHI_HOSTCONF_SUPPORTED_MASK;
        return VAL_SUCCESS;
    case RHI_HOSTCONF_GET_IPA_CHANGE_ALIGNMENT:
        rec_enter->gprs[0] = RHI_HOSTCONF_IPA_CHANGE_ALIGNMENT;
        return VAL_SUCCESS;

    default:
        /* Unknown FID */
        LOG(ERROR, "RHI invalid fid: %lx\n", fid);
        return VAL_ERROR;
    }
}
