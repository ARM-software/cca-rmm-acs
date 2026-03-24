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

struct ifr_step {
    const char *desc;
    uint64_t vdev_id;
    uint64_t expected;
    bool expect_handle;
};

static uint64_t rhi_da_vdev_continue(uint64_t vdev_id, uint64_t handle,
                                     uint64_t expected)
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

static uint64_t rhi_da_vdev_get_interface_report(uint64_t vdev_id, uint64_t expected,
                                                 uint64_t *handle)
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

    if (expected == RHI_DA_ERROR_INCOMPLETE) {
        if ((rhi_call.gprs[0] != RHI_DA_ERROR_INCOMPLETE) &&
            (rhi_call.gprs[0] != RHI_DA_SUCCESS))
        {
            LOG(ERROR, "RHI status code mismatch, expected:%lx or %lx received: %lx\n",
                (uint64_t)RHI_DA_ERROR_INCOMPLETE,
                (uint64_t)RHI_DA_SUCCESS,
                rhi_call.gprs[0]);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            return VAL_ERROR;
        }
    } else if (rhi_call.gprs[0] != expected) {
        LOG(ERROR, "RHI status code mismatch, expected:%lx received: %lx\n",
            expected, rhi_call.gprs[0]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        return VAL_ERROR;
    }

    if ((rhi_call.gprs[0] == RHI_DA_ERROR_INCOMPLETE) && handle) {
        *handle = rhi_call.gprs[1];
        if (*handle == 0) {
            LOG(ERROR, "RHI_DA_VDEV_GET_INTERFACE_REPORT returned zero handle\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            return VAL_ERROR;
        }
    }

    return VAL_SUCCESS;
}

void rhi_da_vdev_get_interface_report_realm(void)
{
    uint64_t ret;
    val_realm_rsi_host_call_t *gv_realm_host_call;
    uint64_t vdev_id;
    uint64_t handle = 0;
    uint64_t i;

    /* Below code is executed for REC[0] only */
    LOG(DBG, "In realm_create_realm REC[0], mpdir=%lx\n", val_read_mpidr());
    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    vdev_id = gv_realm_host_call->gprs[1];

    const struct ifr_step steps[] = {
        {"invalid vdev id", INVALID_VDEV_ID, RHI_DA_ERROR_INVALID_VDEV_ID, false},
        {"valid request", vdev_id, RHI_DA_ERROR_INCOMPLETE, true},
    };

    for (i = 0; i < (sizeof(steps) / sizeof(steps[0])); i++)
    {
        LOG(TEST, "Validate RHI_DA_VDEV_GET_INTERFACE_REPORT %s:\n", steps[i].desc);
        ret = rhi_da_vdev_get_interface_report(steps[i].vdev_id, steps[i].expected,
                                               steps[i].expect_handle ? &handle : NULL);
        if (ret)
            goto exit;
    }

    /* Continue loop until the operation completes */
    for (;;) {
        uint64_t rc = rhi_da_vdev_continue(vdev_id, handle, RHI_DA_ERROR_INCOMPLETE);
        if (rc == RHI_DA_SUCCESS)
            break;
        if (rc == VAL_ERROR) {
            ret = VAL_ERROR;
            goto exit;
        }
    }

    LOG(TEST, "RHI_DA_VDEV_GET_INTERFACE_REPORT completed\n");

exit:
    val_realm_return_to_host();
}
