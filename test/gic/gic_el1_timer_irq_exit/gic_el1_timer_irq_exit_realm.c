/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"

void gic_el1_timer_irq_exit_realm(void)
{
    gic_realm_el1_timer_fire();
}
