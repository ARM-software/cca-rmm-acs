/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"
#include "val_realm_framework.h"
#include "val_timer.h"

#define GIC_EL2_TIMER_WAIT_MS 20U

void gic_el2_timer_asserts_realm(void)
{
    uint64_t time;

    val_realm_return_to_host();

    time = val_sleep_elapsed_time(GIC_EL2_TIMER_WAIT_MS);

    (void)time;
    val_realm_return_to_host();
}
