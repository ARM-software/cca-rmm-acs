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

#define INVALID_VDEV_ID 0xFFFFFFFF
#define INVALID_HANDLE 0xFFFFFFFF

static uint64_t rhi_da_vdev_continue(uint64_t vdev_id, uint64_t handle, uint64_t expected)
{
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t rhi_call = {0};
    uint64_t ret;

    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_DA_VDEV_CONTINUE;
    rhi_call.gprs[1] = vdev_id;
    rhi_call.gprs[2] = handle;
    ret = val_realm_rsi_rhi_host(&rhi_call);
    if (ret)
    {
        LOG(ERROR, "RSI_HOST_CALL failed: %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return VAL_ERROR;
    }

    if (expected == RHI_DA_ERROR_INCOMPLETE) {
        if ((rhi_call.gprs[0] != RHI_DA_ERROR_INCOMPLETE) &&
            (rhi_call.gprs[0] != RHI_DA_SUCCESS))
        {
            LOG(ERROR, "RHI status code mismatch, expected:%lx or %lx received: %lx\n",
                (uint64_t)RHI_DA_ERROR_INCOMPLETE,
                (uint64_t)RHI_DA_SUCCESS,
                rhi_call.gprs[0]);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            return VAL_ERROR;
        }
    } else if (rhi_call.gprs[0] != expected) {
        LOG(ERROR, "RHI status code mismatch, expected:%lx received: %lx\n",
            expected, rhi_call.gprs[0]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return VAL_ERROR;
    }

    return (rhi_call.gprs[0] == RHI_DA_SUCCESS) ? RHI_DA_SUCCESS : RHI_DA_ERROR_INCOMPLETE;
}

static uint64_t rhi_da_start_long_op(uint64_t vdev_id, uint64_t *handle)
{
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t rhi_call = {0};
    uint64_t ret;

    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_DA_VDEV_GET_INTERFACE_REPORT;
    rhi_call.gprs[1] = vdev_id;
    ret = val_realm_rsi_rhi_host(&rhi_call);
    if (ret)
    {
        LOG(ERROR, "RSI_HOST_CALL failed: %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        return VAL_ERROR;
    }

    if (rhi_call.gprs[0] != RHI_DA_ERROR_INCOMPLETE)
    {
        LOG(ERROR, "RHI status code mismatch, expected:%lx received: %lx\n",
            (uint64_t)RHI_DA_ERROR_INCOMPLETE, rhi_call.gprs[0]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        return VAL_ERROR;
    }

    *handle = rhi_call.gprs[1];
    if (*handle == 0) {
        LOG(ERROR, "RHI_DA_VDEV_GET_INTERFACE_REPORT returned zero handle\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        return VAL_ERROR;
    }

    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_DA_VDEV_CONTINUE;
    rhi_call.gprs[1] = vdev_id;
    rhi_call.gprs[2] = *handle;
    ret = val_realm_rsi_rhi_host(&rhi_call);
    if (ret)
    {
        LOG(ERROR, "RSI_HOST_CALL failed: %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        return VAL_ERROR;
    }

    if ((rhi_call.gprs[0] != RHI_DA_ERROR_INCOMPLETE) &&
        (rhi_call.gprs[0] != RHI_DA_SUCCESS))
    {
        LOG(ERROR, "RHI status code mismatch, expected:%lx or %lx received: %lx\n",
            (uint64_t)RHI_DA_ERROR_INCOMPLETE,
            (uint64_t)RHI_DA_SUCCESS,
            rhi_call.gprs[0]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void rhi_da_vdev_continue_realm(void)
{
    uint64_t ret;
    val_realm_rsi_host_call_t *gv_realm_host_call;
    uint64_t vdev_id;
    uint64_t handle = 0;
    uint64_t i;

    struct cont_step {
        const char *desc;
        uint64_t action;
    };

    enum {
        STEP_START_LONG_OP = 0,
        STEP_CONTINUE_INVALID_VDEV = 1,
        STEP_CONTINUE_INVALID_HANDLE = 2,
        STEP_CONTINUE_LOOP = 3
    };

    /* Below code is executed for REC[0] only */
    LOG(DBG, "In realm_create_realm REC[0], mpdir=%lx\n", val_read_mpidr());
    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    vdev_id = gv_realm_host_call->gprs[1];

    const struct cont_step steps[] = {
        {"start long-running operation", STEP_START_LONG_OP},
        {"continue with invalid vdev id", STEP_CONTINUE_INVALID_VDEV},
        {"continue with invalid handle", STEP_CONTINUE_INVALID_HANDLE},
        {"continue loop to completion", STEP_CONTINUE_LOOP},
    };

    for (i = 0; i < (sizeof(steps) / sizeof(steps[0])); i++)
    {
        LOG(TEST, "Validate RHI_DA_VDEV_CONTINUE %s:\n", steps[i].desc);
        switch (steps[i].action) {
        case STEP_START_LONG_OP:
            ret = rhi_da_start_long_op(vdev_id, &handle);
            break;
        case STEP_CONTINUE_INVALID_VDEV:
            ret = rhi_da_vdev_continue(INVALID_VDEV_ID, handle,
                                       RHI_DA_ERROR_INVALID_VDEV_ID);
            break;
        case STEP_CONTINUE_INVALID_HANDLE:
            ret = rhi_da_vdev_continue(vdev_id, INVALID_HANDLE,
                                       RHI_DA_ERROR_DEVICE);
            break;
        case STEP_CONTINUE_LOOP: {
            uint64_t rc;
            do {
                rc = rhi_da_vdev_continue(vdev_id, handle, RHI_DA_ERROR_INCOMPLETE);
                if (rc == VAL_ERROR) {
                    ret = VAL_ERROR;
                    break;
                }
            } while (rc == RHI_DA_ERROR_INCOMPLETE);
            ret = (rc == RHI_DA_SUCCESS) ? VAL_SUCCESS : VAL_ERROR;
            break;
        }
        default:
            ret = VAL_ERROR;
            break;
        }

        if (ret)
            goto exit;
    }

    LOG(TEST, "RHI_DA_VDEV_CONTINUE completed\n");

exit:
    val_realm_return_to_host();
}
