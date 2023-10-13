/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_irq.h"

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

    val_host_realm_params(&realm);

    /* Populate realm with one REC */
    if (val_host_realm_setup(&realm, true))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);

    if (val_irq_register_handler(PLATFORM_NS_WD_INTR, wd_irq_handler))
    {
        LOG(ERROR, "\tWD interrupt register failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    val_irq_enable(PLATFORM_NS_WD_INTR, 0);

    val_ns_wdog_enable(100);

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
        LOG(ALWAYS, "\tWD Interrupt triggered\n", 0, 0);
    } else {
        LOG(ERROR, "\tWD interrupt not triggered\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto free_irq;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
free_irq:
    val_irq_disable(PLATFORM_NS_WD_INTR);

    if (val_irq_unregister_handler(PLATFORM_NS_WD_INTR))
    {
        LOG(ERROR, "\tWD interrupt unregister failed\n", 0, 0);
    }

destroy_realm:
    return;
}
