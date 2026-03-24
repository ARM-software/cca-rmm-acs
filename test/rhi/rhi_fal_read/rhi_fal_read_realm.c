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

#define FAL_PATTERN 0xabcdabcdU

static void build_expected(uint8_t *buffer, uint64_t length)
{
    uint64_t idx;
    uint32_t pattern = FAL_PATTERN;

    for (idx = 0; idx < length; idx++)
    {
        buffer[idx] = ((uint8_t *)&pattern)[idx % sizeof(pattern)];
    }
}

void rhi_fal_read_realm(void)
{
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t rhi_call = {0};
    uint64_t ret;
    uint64_t buffer_ipa;
    uint8_t expected[RHI_FAL_LOG_SIZE];

    buffer_ipa = (uint64_t)(val_get_shared_region_base() + TEST_USE_OFFSET1);
    val_memset((void *)buffer_ipa, 0, PAGE_SIZE);
    build_expected(expected, RHI_FAL_LOG_SIZE);

    LOG(TEST, "Validate RHI_FAL_READ invalid buffer\n");
    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_FAL_READ;
    rhi_call.gprs[1] = buffer_ipa + 1;
    rhi_call.gprs[2] = PAGE_SIZE;
    ret = val_realm_rsi_rhi_host(&rhi_call);
    if (ret)
    {
        LOG(ERROR, "RSI_HOST_CALL failed: %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    if (rhi_call.gprs[0] != RHI_FAL_ACCESS_FAILED)
    {
        LOG(ERROR, "RHI_FAL_READ expected access failed, received: %lx\n", rhi_call.gprs[0]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    LOG(TEST, "RHI_FAL_READ full log\n");
    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_FAL_READ;
    rhi_call.gprs[1] = buffer_ipa;
    rhi_call.gprs[2] = PAGE_SIZE;
    ret = val_realm_rsi_rhi_host(&rhi_call);
    if (ret)
    {
        LOG(ERROR, "RSI_HOST_CALL failed: %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    if (rhi_call.gprs[0] != RHI_FAL_SUCCESS ||
        rhi_call.gprs[1] != RHI_FAL_LOG_SIZE ||
        rhi_call.gprs[2] != 0)
    {
        LOG(ERROR, "RHI_FAL_READ failed: %lx bytes %lx remaining %lx\n",
                                            rhi_call.gprs[0], rhi_call.gprs[1], rhi_call.gprs[2]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    if (val_memcmp((void *)buffer_ipa, expected, RHI_FAL_LOG_SIZE))
    {
        LOG(ERROR, "RHI_FAL_READ data mismatch\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_FAL_READ;
    rhi_call.gprs[1] = buffer_ipa;
    rhi_call.gprs[2] = PAGE_SIZE;
    ret = val_realm_rsi_rhi_host(&rhi_call);
    if (ret)
    {
        LOG(ERROR, "RSI_HOST_CALL failed: %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    if (rhi_call.gprs[0] != RHI_FAL_SUCCESS ||
        rhi_call.gprs[1] != 0 ||
        rhi_call.gprs[2] != 0)
    {
        LOG(ERROR, "RHI_FAL_READ expected empty: %lx bytes %lx remaining %lx\n",
                                            rhi_call.gprs[0], rhi_call.gprs[1], rhi_call.gprs[2]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    LOG(TEST, "RHI_FAL_READ completed\n");

exit:
    val_realm_return_to_host();
}
