/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_timer.h"
#include "val_host_rmi.h"
#include "val_irq.h"

static volatile int handler_flag;

static int timer_handler(void)
{
    val_disable_phy_timer_el2();
    handler_flag = 1;

    return 0;
}

void gic_timer_nsel2_trig_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;
    val_host_rec_exit_ts *rec_exit = NULL;

    if (val_irq_register_handler(IRQ_PHY_TIMER_EL2, timer_handler))
    {
        LOG(ERROR, "\tIRQ_PHY_TIMER_EL2 interrupt register failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    val_irq_enable(IRQ_PHY_TIMER_EL2, 0);

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    /* Populate realm with one REC */
    if (val_host_realm_setup(&realm, true))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto free_irq;
    }
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);
    /* Program and enable the EL2 timer */
    val_timer_set_phy_el2(60);

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto free_irq;
    }

    if ((rec_exit->exit_reason != RMI_EXIT_IRQ) || rec_exit->esr)
    {
        LOG(ERROR, "\tRec exit params mismatch, exit_reason=%x esr %lx\n",
                        rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto free_irq;
    }

    if (handler_flag == 1)
    {
        LOG(ALWAYS, "\ttimer Interrupt triggered\n", 0, 0);
    } else {
        LOG(ERROR, "\ttimer interrupt not triggered\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto free_irq;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
free_irq:
    val_irq_disable(IRQ_PHY_TIMER_EL2);

    if (val_irq_unregister_handler(IRQ_PHY_TIMER_EL2))
    {
        LOG(ERROR, "\tIRQ_PHY_TIMER_EL2 interrupt unregister failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
    }

destroy_realm:
    return;
}
