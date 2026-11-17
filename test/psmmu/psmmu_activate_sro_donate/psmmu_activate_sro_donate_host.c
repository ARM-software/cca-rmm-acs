/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "psmmu_sro_common.h"

void psmmu_activate_sro_donate_host(void)
{
    psmmu_sro_context_ts ctx;
    psmmu_sro_result_ts sro;
    uint32_t rc;
    uint64_t ret;

    rc = psmmu_sro_context_init_psmmu_only(&ctx);
    if (rc == VAL_SKIP_CHECK) {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }
    if (rc != VAL_SUCCESS) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    ret = psmmu_sro_activate_prepare(&ctx, &sro);
    if (ret == RMI_INCOMPLETE) {
        LOG(ERROR, "PSMMU activation SRO could not be completed.\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto cleanup;
    }
    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "PSMMU activation failed, ret=0x%lx.\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto cleanup;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    psmmu_sro_context_cleanup(&ctx);
}
