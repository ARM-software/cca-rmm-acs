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

void rhi_hostconf_get_ipa_change_alignment_realm(void)
{
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t rhi_call = {0};
    uint64_t ret;

    LOG(DBG, "Query the RHI_HOSTCONF_GET_IPA_CHANGE_ALIGNMENT:\n");
    rhi_call.imm = 0;
    rhi_call.gprs[0] = RHI_HOSTCONF_GET_IPA_CHANGE_ALIGNMENT;
    ret = val_realm_rsi_rhi_host(&rhi_call);
    if (ret)
    {
        LOG(ERROR, "RSI_HOST_CALL failed: %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    if (rhi_call.gprs[0] != RHI_HOSTCONF_IPA_CHANGE_ALIGNMENT)
    {
        LOG(ERROR, "RHI hostconf ipa change alignment mismatch, expected:%lx received: %lx\n",
                                            RHI_HOSTCONF_IPA_CHANGE_ALIGNMENT, rhi_call.gprs[0]);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    LOG(DBG, "RHI_HOSTCONF_GET_IPA_CHANGE_ALIGNMENT: %lx\n", rhi_call.gprs[0]);

exit:
    val_realm_return_to_host();
}


