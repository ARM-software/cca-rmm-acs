/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "psmmu_sro_common.h"

void dpt_l1_destroy_sro_reclaim_host(void)
{
    psmmu_sro_context_ts ctx;
    psmmu_sro_result_ts sro;
    uint32_t rc;
    uint64_t ret;

    rc = psmmu_sro_context_init(&ctx);
    if (rc == VAL_SKIP_CHECK) {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }
    if (rc != VAL_SUCCESS) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    ret = psmmu_sro_dpt_l0_create(&ctx, 0, &sro);
    if (ret == RMI_ERROR_NOT_SUPPORTED) {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto cleanup;
    }
    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "DPT L0 create failed, ret=0x%lx.\n", ret);
        if (ret == RMI_INCOMPLETE) {
            val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        } else {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        }
        goto cleanup;
    }

    ret = psmmu_sro_dpt_l1_create(&ctx, ctx.l1_addr, &sro);
    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "DPT L1 create failed, ret=0x%lx.\n", ret);
        if (ret == RMI_INCOMPLETE) {
            val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        } else {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        }
        goto cleanup;
    }

    ret = psmmu_sro_dpt_l1_destroy(&ctx, ctx.l1_addr, &sro);
    if (ret == RMI_INCOMPLETE) {
        LOG(ERROR, "DPT L1 destroy SRO could not be completed.\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto cleanup;
    }
    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "DPT L1 destroy failed, ret=0x%lx.\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto cleanup;
    }

    ret = psmmu_sro_dpt_l1_destroy(&ctx, ctx.l1_addr, &sro);
    if (ret != RMI_ERROR_INPUT) {
        LOG(ERROR, "Expected duplicate DPT L1 destroy input error, got 0x%lx.\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto cleanup;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    psmmu_sro_context_cleanup(&ctx);
}
