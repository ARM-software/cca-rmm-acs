/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"
#include "val_host_realm.h"
#include "val_irq.h"
#include "val_timer.h"

#define GIC_EL2_TIMER_TIMEOUT 10000000U

static volatile int gic_timer_handler_flag;

static int gic_el2_timer_handler(void)
{
    val_disable_phy_timer_el2();
    gic_timer_handler_flag = 1;

    return 0;
}

void gic_el2_timer_asserts_host(void)
{
    val_host_realm_ts realm;
    val_host_rec_exit_ts *rec_exit;
    uint64_t ret;

    gic_timer_handler_flag = 0;
    val_disable_phy_timer_el2();

    if (val_irq_register_handler(IRQ_PHY_TIMER_EL2, gic_el2_timer_handler)) {
        LOG(ERROR, "IRQ_PHY_TIMER_EL2 interrupt register failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    val_irq_enable(IRQ_PHY_TIMER_EL2, 0);

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    if (val_host_realm_setup(&realm, 1)) {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto free_irq;
    }

    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto free_irq;
    }

    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "Unexpected REC exit, exit_reason=%x esr=%lx far=%lx hpfar=%lx\n",
            rec_exit->exit_reason, rec_exit->esr, rec_exit->far, rec_exit->hpfar);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto free_irq;
    }

    val_timer_set_phy_el2(GIC_EL2_TIMER_TIMEOUT);

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto free_irq;
    }

    if ((rec_exit->exit_reason != RMI_EXIT_IRQ) || (rec_exit->esr != 0U) ||
        (gic_timer_handler_flag == 0)) {
        LOG(ERROR, "EL2 timer output did not assert as expected, "
                   "exit_reason=%x esr=%lx far=%lx hpfar=%lx\n",
            rec_exit->exit_reason, rec_exit->esr, rec_exit->far, rec_exit->hpfar);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto free_irq;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

free_irq:
    val_irq_disable(IRQ_PHY_TIMER_EL2);
    if (val_irq_unregister_handler(IRQ_PHY_TIMER_EL2)) {
        LOG(ERROR, "IRQ_PHY_TIMER_EL2 interrupt unregister failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
    }
}
