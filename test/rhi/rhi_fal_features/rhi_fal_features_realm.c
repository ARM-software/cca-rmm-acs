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

void rhi_fal_features_realm(void)
{
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t rhi_call = {0};
    uint64_t ret;

    LOG(DBG, "Query the RHI_FAL_FEATURES:\n");
    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_FAL_FEATURES;
    ret = val_realm_rsi_rhi_host(&rhi_call);
    if (ret)
    {
        LOG(ERROR, "RSI_HOST_CALL failed: %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    if (rhi_call.gprs[0] != RHI_FAL_SUPPORTED_MASK)
    {
        LOG(ERROR, "RHI fal features mismatch, expected:%lx received: %lx\n",
                                                    RHI_FAL_SUPPORTED_MASK, rhi_call.gprs[0]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    LOG(DBG, "RHI_FAL_FEATURES: %lx\n", rhi_call.gprs[0]);

exit:
    val_realm_return_to_host();
}


