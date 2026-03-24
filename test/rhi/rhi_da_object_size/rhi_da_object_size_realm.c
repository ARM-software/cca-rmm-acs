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
#define INVALID_OBJECT_TYPE 0xFFFFFFFF

void rhi_da_object_size_realm(void)
{
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t rhi_call = {0};
    uint64_t ret;
    val_realm_rsi_host_call_t *gv_realm_host_call;
    uint64_t vdev_id;

    /* Below code is executed for REC[0] only */
    LOG(DBG, "In realm_create_realm REC[0], mpdir=%lx\n", val_read_mpidr());
    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    vdev_id = gv_realm_host_call->gprs[1];

    LOG(TEST, "Validate RHI_DA_OBJECT_SIZE invalid vdev id:\n");
    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_DA_OBJECT_SIZE;
    rhi_call.gprs[1] = INVALID_VDEV_ID;
    rhi_call.gprs[2] = RHI_DA_DEV_VCA;
    ret = val_realm_rsi_rhi_host(&rhi_call);
    if (ret)
    {
        LOG(ERROR, "RSI_HOST_CALL failed: %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    if (rhi_call.gprs[0] != RHI_DA_ERROR_INVALID_VDEV_ID)
    {
        LOG(ERROR, "RHI status code mismatch, expected:%lx received: %lx\n",
                                            RHI_DA_ERROR_INVALID_VDEV_ID, rhi_call.gprs[0]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    LOG(TEST, "Validate RHI_DA_OBJECT_SIZE invalid object type:\n");
    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_DA_OBJECT_SIZE;
    rhi_call.gprs[1] = vdev_id;
    rhi_call.gprs[2] = INVALID_OBJECT_TYPE;
    ret = val_realm_rsi_rhi_host(&rhi_call);
    if (ret)
    {
        LOG(ERROR, "RSI_HOST_CALL failed: %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    if (rhi_call.gprs[0] != RHI_DA_ERROR_INVALID_OBJECT)
    {
        LOG(ERROR, "RHI status code mismatch, expected:%lx received: %lx\n",
                                            RHI_DA_ERROR_INVALID_OBJECT, rhi_call.gprs[0]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    LOG(TEST, "Validate RHI_DA_OBJECT_SIZE unsupported object type:\n");
    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_DA_OBJECT_SIZE;
    rhi_call.gprs[1] = vdev_id;
    rhi_call.gprs[2] = RHI_DA_DEV_EXTENSION_EVIDENCE;
    ret = val_realm_rsi_rhi_host(&rhi_call);
    if (ret)
    {
        LOG(ERROR, "RSI_HOST_CALL failed: %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    if (rhi_call.gprs[0] != RHI_DA_ERROR_OBJECT_UNSUPPORTED)
    {
        LOG(ERROR, "RHI status code mismatch, expected:%lx received: %lx\n",
                                            RHI_DA_ERROR_OBJECT_UNSUPPORTED, rhi_call.gprs[0]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    LOG(TEST, "Get the RHI_DA_OBJECT_SIZE:\n");
    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_DA_OBJECT_SIZE;
    rhi_call.gprs[1] = vdev_id;
    rhi_call.gprs[2] = RHI_DA_DEV_VCA;
    ret = val_realm_rsi_rhi_host(&rhi_call);
    if (ret)
    {
        LOG(ERROR, "RSI_HOST_CALL failed: %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    if (rhi_call.gprs[0] != RHI_DA_SUCCESS)
    {
        LOG(ERROR, "RHI status code mismatch, expected:%lx received: %lx\n",
                                                    RHI_DA_SUCCESS, rhi_call.gprs[0]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    if (rhi_call.gprs[1] == 0)
    {
        LOG(ERROR, "RHI_DA_OBJECT_SIZE returned zero length\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

    LOG(TEST, "RHI_DA_OBJECT_SIZE completed\n");

exit:
    val_realm_return_to_host();
}
