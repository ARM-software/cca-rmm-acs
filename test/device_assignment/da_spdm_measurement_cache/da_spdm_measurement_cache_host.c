/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "da_spdm_measurement_host.h"
#include "val_host_alloc.h"
#include "val_host_da.h"
#include "val_host_doe.h"
#include "val_host_pcie.h"
#include "val_host_rmi.h"

#define MAX_MEASUREMENT_COMM_CALLS  256U

typedef struct {
    uint32_t request_count;
    uint32_t response_count;
    uint32_t signed_request_count;
    uint32_t signed_response_count;
    bool waiting_for_response;
    bool request_signed;
    bool last_response_signed;
    bool valid;
} measurement_cache_order_ts;

static void check_and_cache_measurement(val_host_vdev_ts *vdev,
                         const da_spdm_measurement_info_ts *info,
                         measurement_cache_order_ts *order,
                         const uint8_t *buffer, size_t offset, size_t len,
                         bool is_response)
{
    const uint8_t *message;

    if ((offset > GRANULE_SIZE) || (len > GRANULE_SIZE - offset) ||
        (len < 4U) || (vdev->meas_len > VAL_HOST_VDEV_MEAS_LEN_MAX) ||
        (len > VAL_HOST_VDEV_MEAS_LEN_MAX - vdev->meas_len)) {
        LOG(ERROR, "Invalid measurement %s cache range: offset=%lu, len=%lu, cached=%lu\n",
            is_response ? "response" : "request", (unsigned long)offset,
            (unsigned long)len, (unsigned long)vdev->meas_len);
        order->valid = false;
        return;
    }

    message = buffer + offset;
    if ((message[0] != info->version) ||
        (message[1] != (is_response ? SPDM_MEASUREMENTS_CODE :
                                      SPDM_GET_MEASUREMENTS_CODE))) {
        LOG(ERROR, "Unexpected cached SPDM %s: version=0x%x, code=0x%x\n",
            is_response ? "response" : "request", message[0], message[1]);
        order->valid = false;
    }

    if (is_response) {
        if (!order->waiting_for_response) {
            LOG(ERROR, "Measurement response cached before its request\n");
            order->valid = false;
        }
        order->response_count++;
        if (order->request_signed)
            order->signed_response_count++;
        order->last_response_signed = order->request_signed;
        order->waiting_for_response = false;
    } else {
        if (order->waiting_for_response) {
            LOG(ERROR, "Measurement request cached before prior response\n");
            order->valid = false;
        }
        order->request_signed = (message[2] & 1U) != 0U;
        if (order->request_signed) {
            if (!info->signed_supported) {
                LOG(ERROR, "Signature requested from unsigned-only device\n");
                order->valid = false;
            }
            order->signed_request_count++;
        }
        order->request_count++;
        order->waiting_for_response = true;
    }

    val_memcpy(vdev->meas + vdev->meas_len, (void *)message, len);
    vdev->meas_len += len;
}

static uint32_t communicate_and_check_measurement_cache(val_host_realm_ts *realm,
                         val_host_pdev_ts *pdev, val_host_vdev_ts *vdev,
                         const da_spdm_measurement_info_ts *info,
                         measurement_cache_order_ts *order)
{
    val_host_dev_comm_enter_ts *enter = &vdev->dev_comm_data->enter;
    val_host_dev_comm_exit_ts *exit = &vdev->dev_comm_data->exit;
    val_smc_param_ts ret;
    size_t response_len;
    uint32_t call;

    enter->status = RMI_DEV_COMM_NONE;
    enter->rsp_len = 0U;

    for (call = 1U; call <= MAX_MEASUREMENT_COMM_CALLS; call++) {
        ret = val_host_rmi_vdev_communicate(realm->rd, pdev->pdev,
                                             vdev->vdev,
                                             (uint64_t)vdev->dev_comm_data);
        if (ret.x0 != RMI_SUCCESS)
            return VAL_ERROR;

        if (exit->flags & (RMI_DEV_COMM_EXIT_FLAGS_REQ_CACHE_BIT |
                           RMI_DEV_COMM_EXIT_FLAGS_RSP_CACHE_BIT)) {
            if (exit->cache_object_id != RMI_DEV_MEASUREMENTS) {
                LOG(ERROR, "Unexpected measurement cache object: %u\n",
                    exit->cache_object_id);
                order->valid = false;
            }

            if (exit->flags & RMI_DEV_COMM_EXIT_FLAGS_REQ_CACHE_BIT)
                check_and_cache_measurement(vdev, info, order,
                     (const uint8_t *)enter->req_addr, exit->req_cache_offset,
                     exit->req_cache_len, false);

            if (exit->flags & RMI_DEV_COMM_EXIT_FLAGS_RSP_CACHE_BIT)
                check_and_cache_measurement(vdev, info, order,
                     (const uint8_t *)enter->rsp_addr, exit->rsp_cache_offset,
                     exit->rsp_cache_len, true);
        }

        if (exit->flags & RMI_DEV_COMM_EXIT_FLAGS_REQ_SEND_BIT) {
            response_len = 0U;
            if (val_host_pcie_doe_communicate(pdev->bdf, pdev->doe_cap_base,
                    (void *)enter->req_addr, exit->req_len,
                    (void *)enter->rsp_addr, GRANULE_SIZE,
                    &response_len, exit->protocol)) {
                enter->status = RMI_DEV_COMM_ERROR;
                enter->rsp_len = 0U;
            } else {
                enter->status = RMI_DEV_COMM_RESPONSE;
                enter->rsp_len = response_len;
            }
        } else {
            enter->status = RMI_DEV_COMM_NONE;
        }

        ret = val_host_rmi_vdev_get_state(vdev->vdev);
        if (ret.x0 != RMI_SUCCESS)
            return VAL_ERROR;
        if ((ret.x1 == RMI_VDEV_UNLOCKED) &&
            val_host_dev_comm_complete(exit->flags))
            return VAL_SUCCESS;
        if ((ret.x1 == RMI_VDEV_ERROR) ||
            (exit->flags & RMI_DEV_COMM_EXIT_FLAGS_STREAM_WAIT_BIT))
            return VAL_ERROR;
    }

    return VAL_ERROR;
}

void da_spdm_measurement_cache_host(void)
{
    val_host_realm_ts realm;
    val_host_pdev_ts pdev;
    val_host_vdev_ts vdev;
    val_host_pdev_flags_ts pdev_flags;
    val_host_vdev_measure_params_ts *params = NULL;
    da_spdm_measurement_info_ts measurement_info;
    measurement_cache_order_ts cache_order;
    val_smc_param_ts ret;
    uint64_t prepared_vdev;
    uint32_t num_bdf, rp_bdf, result, pair_count;

    val_memset(&realm, 0, sizeof(realm));
    val_memset(&pdev, 0, sizeof(pdev));
    val_memset(&vdev, 0, sizeof(vdev));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));
    val_memset(&measurement_info, 0, sizeof(measurement_info));
    val_memset(&cache_order, 0, sizeof(cache_order));

    if (!val_host_rmm_supports_da()) {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    num_bdf = val_pcie_get_num_bdf();
    if ((num_bdf == VAL_ERROR) ||
        val_pcie_find_doe_capability(&num_bdf, &pdev.bdf,
                                     &pdev.doe_cap_base) ||
        val_pcie_get_rootport(pdev.bdf, &rp_bdf)) {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    pdev.root_id = (uint16_t)rp_bdf;
    val_host_set_pdev_flags(&pdev_flags);
    pdev.pdev_flags = pdev_flags;
    vdev.vmid = 1U;
    vdev.vdev_id = 1U;
    vdev.rec_count = 1U;

    prepared_vdev = g_vdev_state_prep_sequence(&pdev, &vdev, &realm,
                                               RMI_VDEV_UNLOCKED);
    if (prepared_vdev == VAL_TEST_PREP_SEQ_FAILED) {
        LOG(ERROR, "SPDM VDEV preparation failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto cleanup;
    }
    vdev.vdev = prepared_vdev;

    result = da_spdm_get_measurement_info(&pdev, &measurement_info);
    if (result == VAL_SKIP_CHECK) {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto cleanup;
    }
    if (result != VAL_SUCCESS) {
        LOG(ERROR, "Invalid cached SPDM negotiation data\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto cleanup;
    }

    vdev.meas = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_VDEV_MEAS_LEN_MAX);
    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((vdev.meas == NULL) || (params == NULL)) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto cleanup;
    }
    val_memset(params, 0, PAGE_SIZE);
    vdev.meas_len = 0U;
    cache_order.valid = true;

    ret = val_host_rmi_vdev_get_measurements(realm.rd, pdev.pdev,
                                             vdev.vdev, (uint64_t)params);
    if (ret.x0 != RMI_SUCCESS) {
        LOG(ERROR, "RMI_VDEV_GET_MEASUREMENTS failed, ret=0x%lx\n",
            (unsigned long)ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto cleanup;
    }

    if (communicate_and_check_measurement_cache(&realm, &pdev, &vdev,
                                                &measurement_info, &cache_order)) {
        LOG(ERROR, "VDEV measurement communication failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto cleanup;
    }

    result = da_spdm_check_measurement_cache(vdev.meas, vdev.meas_len,
                 &measurement_info, NULL, NULL, &pair_count);
    if ((result != VAL_SUCCESS) || !cache_order.valid ||
        (pair_count == 0U) ||
        (pair_count != cache_order.request_count) ||
        (pair_count != cache_order.response_count) ||
        cache_order.waiting_for_response ||
        (measurement_info.signed_supported &&
         ((cache_order.signed_request_count != 1U) ||
          (cache_order.signed_response_count != 1U) ||
          !cache_order.last_response_signed))) {
        LOG(ERROR, "Measurement cache check: parse=%u, valid=%u, pairs=%u, "
            "requests=%u, responses=%u, pending=%u, signed_supported=%u, "
            "signed_requests=%u, signed_responses=%u, final_signed=%u, bytes=%lu\n",
            result, cache_order.valid, pair_count, cache_order.request_count,
            cache_order.response_count, cache_order.waiting_for_response,
            measurement_info.signed_supported, cache_order.signed_request_count,
            cache_order.signed_response_count, cache_order.last_response_signed,
            (unsigned long)vdev.meas_len);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto cleanup;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    if ((vdev.vdev != 0U) &&
        val_host_vdev_teardown(&realm, &pdev, &vdev)) {
        LOG(ERROR, "VDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
    }
    if ((pdev.pdev != 0U) &&
        (pdev.pdev != VAL_TEST_PREP_SEQ_FAILED) &&
        val_host_pdev_teardown(&pdev, pdev.pdev)) {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
    }
    if (params != NULL)
        val_host_mem_free(params);
    if (vdev.meas != NULL)
        val_host_mem_free(vdev.meas);
}
