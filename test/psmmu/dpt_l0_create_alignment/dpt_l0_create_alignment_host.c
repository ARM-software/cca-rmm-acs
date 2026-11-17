/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "psmmu_sro_common.h"

void dpt_l0_create_alignment_host(void)
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

    ret = psmmu_sro_dpt_l0_create(&ctx, 1, &sro);
    if (ret == RMI_ERROR_NOT_SUPPORTED) {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto cleanup;
    }
    if (ret == RMI_INCOMPLETE) {
        LOG(TEST, "DPT L0 create alignment path could not be completed.\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto cleanup;
    }
    if (sro.saw_donate == 0U) {
        LOG(TEST, "No DPT L0 donation request was observed; alignment rule not exercised.\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto cleanup;
    }
    if ((ret != RMI_ERROR_INPUT) || (sro.saw_alignment_failure == 0U)) {
        LOG(ERROR, "Misaligned DPT L0 donation was not rejected, ret=0x%lx.\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto cleanup;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    psmmu_sro_context_cleanup(&ctx);
}
