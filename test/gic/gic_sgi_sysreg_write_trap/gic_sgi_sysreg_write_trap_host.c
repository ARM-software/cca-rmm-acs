/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"

void gic_sgi_sysreg_write_trap_host(void)
{
    val_host_realm_ts realm;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    if (val_host_realm_setup(&realm, 1)) {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    if (gic_host_expect_sysreg_exit(&realm) != VAL_SUCCESS) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return;
    }

    if (gic_host_expect_sysreg_exit(&realm) != VAL_SUCCESS) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        return;
    }

    if (gic_host_expect_sysreg_exit(&realm) != VAL_SUCCESS) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        return;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));
}
