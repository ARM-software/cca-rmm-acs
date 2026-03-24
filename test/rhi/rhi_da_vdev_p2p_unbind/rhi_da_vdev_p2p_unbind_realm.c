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

struct p2p_unbind_step {
    const char *desc;
    uint64_t vdev_id_1;
    uint64_t vdev_id_2;
    uint64_t expected;
    bool expect_handle;
};

static uint64_t rhi_da_vdev_continue_until_complete(uint64_t vdev_id, uint64_t handle)
{
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t rhi_call = {0};
    uint64_t ret;

    for (;;) {
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

        if (rhi_call.gprs[0] == RHI_DA_SUCCESS)
            return VAL_SUCCESS;
        if (rhi_call.gprs[0] != RHI_DA_ERROR_INCOMPLETE)
        {
            LOG(ERROR, "RHI status code mismatch, expected:%lx or %lx received: %lx\n",
                (uint64_t)RHI_DA_ERROR_INCOMPLETE,
                (uint64_t)RHI_DA_SUCCESS,
                rhi_call.gprs[0]);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            return VAL_ERROR;
        }
    }
}

static uint64_t rhi_da_vdev_p2p_unbind_request(uint64_t vdev_id_1, uint64_t vdev_id_2,
                                               uint64_t expected,
                                               uint64_t *handle)
{
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t rhi_call = {0};
    uint64_t ret;

    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_DA_VDEV_P2P_UNBIND;
    rhi_call.gprs[1] = vdev_id_1;
    rhi_call.gprs[2] = vdev_id_2;
    ret = val_realm_rsi_rhi_host(&rhi_call);
    if (ret)
    {
        LOG(ERROR, "RSI_HOST_CALL failed: %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        return VAL_ERROR;
    }

    if (rhi_call.gprs[0] != expected)
    {
        LOG(ERROR, "RHI status code mismatch, expected:%lx received: %lx\n",
            expected, rhi_call.gprs[0]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        return VAL_ERROR;
    }

    if ((expected == RHI_DA_ERROR_INCOMPLETE) && handle)
    {
        *handle = rhi_call.gprs[1];
        if (*handle == 0)
        {
            LOG(ERROR, "RHI_DA_VDEV_P2P_UNBIND returned zero handle\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            return VAL_ERROR;
        }
    }

    return VAL_SUCCESS;
}

void rhi_da_vdev_p2p_unbind_realm(void)
{
    uint64_t ret;
    val_realm_rsi_host_call_t *gv_realm_host_call;
    uint64_t vdev_id_1;
    uint64_t vdev_id_2;
    uint64_t handle = 0;
    uint64_t i;

    /* Below code is executed for REC[0] only */
    LOG(DBG, "In realm_create_realm REC[0], mpdir=%lx\n", val_read_mpidr());
    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    vdev_id_1 = gv_realm_host_call->gprs[1];
    vdev_id_2 = gv_realm_host_call->gprs[2];

    const struct p2p_unbind_step steps[] = {
        {"invalid vdev id", vdev_id_1, INVALID_VDEV_ID, RHI_DA_ERROR_INVALID_VDEV_ID, false},
        {"valid request", vdev_id_1, vdev_id_2, RHI_DA_ERROR_INCOMPLETE, true},
    };

    for (i = 0; i < (sizeof(steps) / sizeof(steps[0])); i++)
    {
        LOG(TEST, "Validate RHI_DA_VDEV_P2P_UNBIND %s:\n", steps[i].desc);
        ret = rhi_da_vdev_p2p_unbind_request(steps[i].vdev_id_1, steps[i].vdev_id_2,
                                             steps[i].expected,
                                             steps[i].expect_handle ? &handle : NULL);
        if (ret)
            goto exit;
    }

    /* Device communication loop must complete for both VDEVs */
    if (rhi_da_vdev_continue_until_complete(vdev_id_1, handle))
        goto exit;
    if (rhi_da_vdev_continue_until_complete(vdev_id_2, handle))
        goto exit;

    LOG(TEST, "RHI_DA_VDEV_P2P_UNBIND completed\n");

exit:
    val_realm_return_to_host();
}
