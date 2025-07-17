/*
 * Copyright (c) 2023, 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_irq.h"
#include "val_timer.h"

static volatile int handler_flag;

static int timer_handler(void)
{
    val_disable_phy_timer_el2();
    handler_flag = 1;

    return 0;
}

void attestation_rec_exit_irq_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret, i = 0, timeout = 1000000;
    val_host_rec_exit_ts *rec_exit = NULL;
    uint64_t *shared_flag1 = (val_get_shared_region_base() + TEST_USE_OFFSET1);
    uint64_t *shared_flag2 = (val_get_shared_region_base() + TEST_USE_OFFSET2);
    uint64_t *shared_flag3 = (val_get_shared_region_base() + TEST_USE_OFFSET3);
    uint64_t *test_shared_region_flag = (val_get_shared_region_base() + TEST_USE_OFFSET4);
    uint64_t *shared_err_flag = (val_get_shared_region_base() + TEST_USE_OFFSET5);

    if (val_irq_register_handler(IRQ_PHY_TIMER_EL2, timer_handler))
    {
        LOG(ERROR, "IRQ_PHY_TIMER_EL2 interrupt register failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    val_irq_enable(IRQ_PHY_TIMER_EL2, 0);

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    /* Populate realm with one REC */
    if (val_host_realm_setup(&realm, true))
    {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto free_irq;
    }

    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto free_irq;
    }

    do {
        /* Program and enable the EL2 timer */
        val_timer_set_phy_el2(timeout);

        ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
        if (ret)
        {
            LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto free_irq;
        }

        if ((rec_exit->exit_reason == RMI_EXIT_HOST_CALL) &&
            *shared_err_flag == 1)
        {
            LOG(ERROR, "Token init/continue failed.\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto free_irq;
        }

        if ((rec_exit->exit_reason == RMI_EXIT_IRQ) &&
            *shared_flag1 == 1 && *shared_flag2 == 1 && *shared_flag3 == 0)
        {
            *test_shared_region_flag = 1;

            if (handler_flag == 1)
            {
                LOG(TEST, "Timer Interrupt triggered\n");
            } else {
                LOG(ERROR, "Timer interrupt not triggered\n");
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
                goto free_irq;
            }

            ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
            if (ret)
            {
                LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
                goto free_irq;
            }
        }

        *shared_flag1 = 0;
        *shared_flag2 = 0;
        *shared_flag3 = 0;
        handler_flag = 0;
        i = i + 1;

        val_disable_phy_timer_el2();

        timeout = timeout + 100000;

    } while (*test_shared_region_flag != 1 && i < 5);

    if (*test_shared_region_flag != 1)
    {
        LOG(ALWAYS, "Tried %d combinations but interrupt didnt triggered \
                                         for exact test scenario\n", i);
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto free_irq;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
free_irq:
    val_irq_disable(IRQ_PHY_TIMER_EL2);

    if (val_irq_unregister_handler(IRQ_PHY_TIMER_EL2))
    {
        LOG(ERROR, "IRQ_PHY_TIMER_EL2 interrupt unregister failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
    }

destroy_realm:
    return;
}
