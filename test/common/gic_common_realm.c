/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"
#include "val_irq.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"
#include "val_timer.h"

#define GIC_TIMER_TIMEOUT 1000000U
#define GIC_COUNTER_TIMEOUT 100U
#define GIC_COUNTER_EQUAL_SAMPLES 8U

uint32_t gic_realm_counter_equal_check(void)
{
    uint64_t phys_counter_before;
    uint64_t phys_counter_after;
    uint64_t virt_counter;
    uint32_t i;

    for (i = 0U; i < GIC_COUNTER_EQUAL_SAMPLES; i++) {
        phys_counter_before = syscounter_read();
        virt_counter = virtualcounter_read();
        phys_counter_after = syscounter_read();

        if ((virt_counter < phys_counter_before) ||
            (virt_counter > phys_counter_after)) {
            LOG(ERROR, "Counter mismatch, phys_before=%lx virt=%lx phys_after=%lx\n",
                phys_counter_before, virt_counter, phys_counter_after);
            return 1U;
        }
    }

    return VAL_SUCCESS;
}

uint32_t gic_realm_counter_check(void)
{
    uint64_t phys_counter_t0, phys_counter_t1;
    uint64_t virt_counter_t0, virt_counter_t1;
    uint64_t timeout = GIC_COUNTER_TIMEOUT + 1U;

    val_timer_set_phy_el1(GIC_COUNTER_TIMEOUT, true);

    phys_counter_t0 = syscounter_read();
    virt_counter_t0 = virtualcounter_read();

    phys_counter_t1 = syscounter_read();
    while ((phys_counter_t0 == phys_counter_t1) && (timeout > 0U)) {
        val_sleep_elapsed_time(1U);
        timeout--;
        phys_counter_t1 = syscounter_read();
    }
    virt_counter_t1 = virtualcounter_read();

    if (phys_counter_t1 <= phys_counter_t0) {
        LOG(ERROR, "Physical counter is not monotonically increasing\n");
        val_disable_phy_timer_el1();
        return 1U;
    }

    if (virt_counter_t1 <= virt_counter_t0) {
        LOG(ERROR, "Virtual counter is not monotonically increasing\n");
        val_disable_phy_timer_el1();
        return 2U;
    }

    val_disable_phy_timer_el1();
    return VAL_SUCCESS;
}

void gic_realm_el1_timer_fire(void)
{
    val_timer_set_phy_el1(GIC_TIMER_TIMEOUT, false);
    val_sleep_elapsed_time(2U);
    val_realm_return_to_host();
}
