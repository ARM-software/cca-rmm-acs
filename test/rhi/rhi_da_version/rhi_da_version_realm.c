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

void rhi_da_version_realm(void)
{
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t rhi_call = {0};
    uint64_t ret;

    LOG(DBG, "Query the RHI_DA_VERSION:\n");
    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_DA_VERSION;
    ret = val_realm_rsi_rhi_host(&rhi_call);
    if (ret)
    {
        LOG(ERROR, "RSI_HOST_CALL failed: %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    if (rhi_call.gprs[0] == RHI_SMC_NOT_SUPPORTED)
    {
        LOG(ERROR, "RHI_DA_VERSION is not supported\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    if (rhi_call.gprs[0] != RHI_DA_VER)
    {
        LOG(ERROR, "RHI da version mismatch, expected:%lx received: %lx\n",
                                                    RHI_DA_VER, rhi_call.gprs[0]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    LOG(DBG, "RHI_DA_VERSION: %lx\n", rhi_call.gprs[0]);

exit:
    val_realm_return_to_host();
}


