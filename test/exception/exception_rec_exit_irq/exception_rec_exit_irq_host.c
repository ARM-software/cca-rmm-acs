/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_irq.h"
#include "val_timer.h"

static volatile int handler_flag;

static int wd_irq_handler(void)
{
    val_ns_wdog_disable();
    handler_flag = 1;

    return 0;
}

void exception_rec_exit_irq_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;
    val_host_rec_exit_ts *rec_exit = NULL;
    val_host_rec_enter_ts *rec_enter = NULL;
    uint64_t cnt_val1, cnt_val2, cnt_frq, boot_time_ms;

    val_host_realm_params(&realm);

    /* Populate realm with one REC */
    if (val_host_realm_setup(&realm, true))
    {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    rec_enter = &(((val_host_rec_run_ts *)realm.run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);

    /* Read Counter before entering realm */
    cnt_val1 = val_read_cntpct_el0();

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    /* Check that REC exit was due to host call after realm is booted */
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "Unexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    /* Read counter after coming back to host */
    cnt_val2 = val_read_cntpct_el0();

    /* Register and enable IRQ handler */
    if (val_irq_register_handler(PLATFORM_NS_WD_INTR, wd_irq_handler))
    {
        LOG(ERROR, "WD interrupt register failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    val_irq_enable(PLATFORM_NS_WD_INTR, 0);

    /* Calculate the boot time of the platform */
    cnt_frq = val_read_cntfrq_el0();
    boot_time_ms = ((cnt_val2 - cnt_val1) * 1000) / cnt_frq;

    /* Enable Watchdog timer and enter Realm */
    val_ns_wdog_enable((uint32_t)(boot_time_ms * 100000));

    rec_enter->gprs[1] = boot_time_ms;

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto free_irq;
    }

    /* Check REC exit is due to IRQ */
    if ((rec_exit->exit_reason != RMI_EXIT_IRQ) || rec_exit->esr)
    {
        LOG(ERROR, "Rec exit params mismatch, exit_reason=%x esr %lx\n",
                        rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto free_irq;
    }

    if (handler_flag == 1)
    {
        LOG(TEST, "WD Interrupt triggered\n");
    } else {
        LOG(ERROR, "WD interrupt not triggered\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto free_irq;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
free_irq:
    val_irq_disable(PLATFORM_NS_WD_INTR);

    if (val_irq_unregister_handler(PLATFORM_NS_WD_INTR))
    {
        LOG(ERROR, "WD interrupt unregister failed\n");
    }

destroy_realm:
    return;
}
