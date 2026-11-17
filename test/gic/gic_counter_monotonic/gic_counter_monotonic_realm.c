/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"
#include "val_realm_framework.h"

void gic_counter_monotonic_realm(void)
{
    uint32_t ret;

    ret = gic_realm_counter_check();
    if (ret == 1U) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
    } else if (ret == 2U) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
    }

    val_realm_return_to_host();
}
