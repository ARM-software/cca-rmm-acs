/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"
#include "val_irq.h"
#include "val_timer.h"

#define GIC_TIMER_STATE_SHARED_OFFSET 0x100U
#define GIC_VIRT_TIMER_EXPECTED_CTL   (ARM_ARCH_TIMER_ENABLE | ARM_ARCH_TIMER_IMASK)

struct gic_timer_state {
    uint64_t cntp_cval;
    uint64_t cntv_cval;
};

void gic_rec_exit_timer_state_host(void)
{
    val_host_realm_ts realm;
    val_host_rec_exit_ts *rec_exit;
    struct gic_timer_state *timer_state;
    uint64_t ret;
    uint32_t exp_cntp_ctl = ARM_ARCH_TIMER_ENABLE | ARM_ARCH_TIMER_ISTATUS;

    val_irq_enable(IRQ_PHY_TIMER_EL1, 0);

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    if (val_host_realm_setup(&realm, 1)) {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);
    timer_state = (struct gic_timer_state *)(val_get_shared_region_base() +
                                             GIC_TIMER_STATE_SHARED_OFFSET);

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return;
    }

    if ((rec_exit->exit_reason != RMI_EXIT_IRQ) ||
        (rec_exit->cntp_ctl != exp_cntp_ctl) ||
        (rec_exit->cntp_cval != timer_state->cntp_cval) ||
        (rec_exit->cntv_ctl != GIC_VIRT_TIMER_EXPECTED_CTL) ||
        (rec_exit->cntv_cval != timer_state->cntv_cval) ||
        (rec_exit->esr != 0U)) {
        LOG(ERROR, "EL1 timer REC exit mismatch, exit_reason=%x esr=%lx "
                   "cntp_ctl=%lx cntp_cval=%lx/%lx cntv_ctl=%lx cntv_cval=%lx/%lx\n",
            rec_exit->exit_reason, rec_exit->esr, rec_exit->cntp_ctl,
            rec_exit->cntp_cval, timer_state->cntp_cval, rec_exit->cntv_ctl,
            rec_exit->cntv_cval, timer_state->cntv_cval);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        return;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));
}
