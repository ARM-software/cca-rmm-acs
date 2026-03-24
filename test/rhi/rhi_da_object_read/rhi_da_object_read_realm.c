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
#include "rhi_da_object_read.h"

#define INVALID_VDEV_ID 0xFFFFFFFF
#define INVALID_OBJECT_TYPE 0xFFFFFFFF
#define INVALID_BUFFER_IPA 0

static uint64_t da_object_read(uint64_t vdev_id, uint64_t obj_type,
                               uint64_t buffer_ipa, uint64_t max_len,
                               uint64_t offset, uint64_t expected_status,
                               bool expect_nonzero_len)
{
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t rhi_call = {0};
    uint64_t ret;

    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_DA_OBJECT_READ;
    rhi_call.gprs[1] = vdev_id;
    rhi_call.gprs[2] = obj_type;
    rhi_call.gprs[3] = buffer_ipa;
    rhi_call.gprs[4] = max_len;
    rhi_call.gprs[5] = offset;
    ret = val_realm_rsi_rhi_host(&rhi_call);
    if (ret)
    {
        LOG(ERROR, "RSI_HOST_CALL failed: %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return VAL_ERROR;
    }

    if (rhi_call.gprs[0] != expected_status)
    {
        LOG(ERROR, "RHI status code mismatch, expected:%lx received: %lx\n",
            expected_status, rhi_call.gprs[0]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return VAL_ERROR;
    }

    if (expected_status == RHI_DA_SUCCESS && expect_nonzero_len && rhi_call.gprs[1] == 0)
    {
        LOG(ERROR, "RHI_DA_OBJECT_READ returned zero length\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void rhi_da_object_read_realm(void)
{
    uint64_t ret;
    val_realm_rsi_host_call_t *gv_realm_host_call;
    uint64_t vdev_id;
    uint64_t buffer_ipa;
    uint64_t i;

    /* Below code is executed for REC[0] only */
    LOG(DBG, "In realm_create_realm REC[0], mpdir=%lx\n", val_read_mpidr());
    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    vdev_id = gv_realm_host_call->gprs[1];

    buffer_ipa = (uint64_t)(val_get_shared_region_base() + TEST_USE_OFFSET1);
    val_memset((void *)buffer_ipa, 0, PAGE_SIZE);

    const struct read_step steps[] = RHI_DA_OBJECT_READ_STEPS(
        vdev_id, buffer_ipa, INVALID_VDEV_ID, INVALID_OBJECT_TYPE, INVALID_BUFFER_IPA);

    for (i = 0; i < (sizeof(steps) / sizeof(steps[0])); i++)
    {
        LOG(TEST, "Validate RHI_DA_OBJECT_READ %s:\n", steps[i].desc);
        ret = da_object_read(steps[i].vdev_id, steps[i].obj_type,
                             steps[i].buffer_ipa, steps[i].max_len,
                             steps[i].offset, steps[i].expected_status,
                             steps[i].expect_nonzero_len);
        if (ret)
            goto exit;
    }

    LOG(TEST, "RHI_DA_OBJECT_READ completed\n");

exit:
    val_realm_return_to_host();
}
