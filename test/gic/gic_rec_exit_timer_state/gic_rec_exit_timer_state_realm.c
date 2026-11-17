/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"
#include "pal_arch_helpers.h"
#include "val_realm_framework.h"
#include "val_timer.h"

#define GIC_TIMER_STATE_SHARED_OFFSET 0x100U
#define GIC_PHY_TIMER_TIMEOUT_NS      1000000U
#define GIC_VIRT_TIMER_TIMEOUT_NS     1000000000U

struct gic_timer_state {
    uint64_t cntp_cval;
    uint64_t cntv_cval;
};

void gic_rec_exit_timer_state_realm(void)
{
    struct gic_timer_state *timer_state;
    uint64_t freq = read_cntfrq_el0();
    uint64_t cntp_cval;
    uint64_t cntv_cval;

    timer_state = (struct gic_timer_state *)(val_get_shared_region_base() +
                                             GIC_TIMER_STATE_SHARED_OFFSET);

    val_disable_phy_timer_el1();
    val_disable_virt_timer_el1();

    cntp_cval = syscounter_read() +
                ((freq * GIC_PHY_TIMER_TIMEOUT_NS) / 1000000000U);
    write_cntp_cval_el0(cntp_cval);
    write_cntp_ctl_el0(ARM_ARCH_TIMER_ENABLE);

    cntv_cval = virtualcounter_read() +
                ((freq * GIC_VIRT_TIMER_TIMEOUT_NS) / 1000000000U);
    write_cntv_cval_el0(cntv_cval);
    write_cntv_ctl_el0(ARM_ARCH_TIMER_ENABLE | ARM_ARCH_TIMER_IMASK);

    timer_state->cntp_cval = cntp_cval;
    timer_state->cntv_cval = cntv_cval;

    val_sleep_elapsed_time(2U);
    val_realm_return_to_host();
}
