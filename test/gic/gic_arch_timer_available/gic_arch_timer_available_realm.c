/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"
#include "pal_arch_helpers.h"
#include "val_realm_framework.h"
#include "val_timer.h"

#define GIC_ARCH_TIMER_TEST_TICKS 0x100000U
#define GIC_ARCH_TIMER_CTL_MASK   (ARM_ARCH_TIMER_ENABLE | ARM_ARCH_TIMER_IMASK)

static uint32_t gic_arch_timer_available_check(void)
{
    uint64_t cntp_cval;
    uint64_t cntv_cval;
    uint64_t cntp_ctl;
    uint64_t cntv_ctl;

    val_disable_phy_timer_el1();
    val_disable_virt_timer_el1();

    cntp_cval = syscounter_read() + GIC_ARCH_TIMER_TEST_TICKS;
    cntv_cval = virtualcounter_read() + GIC_ARCH_TIMER_TEST_TICKS;

    write_cntp_cval_el0(cntp_cval);
    write_cntv_cval_el0(cntv_cval);
    write_cntp_ctl_el0(GIC_ARCH_TIMER_CTL_MASK);
    write_cntv_ctl_el0(GIC_ARCH_TIMER_CTL_MASK);
    isb();

    if (read_cntp_cval_el0() != cntp_cval)
        return 1U;

    if (read_cntv_cval_el0() != cntv_cval)
        return 2U;

    cntp_ctl = read_cntp_ctl_el0();
    cntv_ctl = read_cntv_ctl_el0();

    val_disable_phy_timer_el1();
    val_disable_virt_timer_el1();

    if ((cntp_ctl & GIC_ARCH_TIMER_CTL_MASK) != GIC_ARCH_TIMER_CTL_MASK)
        return 3U;

    if ((cntv_ctl & GIC_ARCH_TIMER_CTL_MASK) != GIC_ARCH_TIMER_CTL_MASK)
        return 4U;

    return VAL_SUCCESS;
}

void gic_arch_timer_available_realm(void)
{
    uint32_t ret;

    ret = gic_arch_timer_available_check();
    if (ret != VAL_SUCCESS)
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(ret + 2U)));

    val_realm_return_to_host();
}
