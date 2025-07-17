/*
 * Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_irq.h"
#include "val_timer.h"
#include "val_host_helpers.h"

static volatile int handler_flag;

static int timer_handler(void)
{
    val_disable_phy_timer_el2();
    handler_flag = 1;

    return 0;
}

void planes_rec_exit_irq_host(void)
{
    static val_host_realm_ts realm;
    val_host_realm_flags1_ts realm_flags;
    uint64_t ret;
    val_host_rec_exit_ts *rec_exit = NULL;
    val_host_rec_enter_ts *rec_enter = NULL;
    uint64_t cnt_val1, cnt_val2, cnt_frq, boot_time_ms;

    /* Skip if RMM do not support planes */
    if (!val_host_rmm_supports_planes())
    {
        LOG(ALWAYS, "Planes feature not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto destroy_realm;
    }

    val_memset(&realm, 0, sizeof(realm));
    val_memset(&realm_flags, 0, sizeof(realm_flags));

    val_host_realm_params(&realm);

    /* Overwrite Realm Parameters */
    realm.num_aux_planes = 1;
    realm_flags.rtt_tree_pp = RMI_FEATURE_TRUE;

    if (val_host_rmm_supports_rtt_tree_single())
        realm_flags.rtt_tree_pp = RMI_FEATURE_FALSE;

    val_memcpy(&realm.flags1, &realm_flags, sizeof(realm.flags1));

    LOG(DBG, " INFO: RTT tree per plane : %d\n", realm_flags.rtt_tree_pp);

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm, 1))
    {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    rec_enter = &(((val_host_rec_run_ts *)realm.run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);

    cnt_val1 = val_read_cntpct_el0();
   /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    /* Check that REC exit was due to S2AP change request */
    if (rec_exit->exit_reason != RMI_EXIT_S2AP_CHANGE) {
        LOG(ERROR, "Unexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    /* Update S2AP for the requested memory range */
    if (val_host_set_s2ap(&realm))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    /* Check that REC exit was due to host call for P0 and P1 is initialized */
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "Unexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    /* Initialize EL2 timer interrupt after P0 and P1 is booted and initialized */
    if (val_irq_register_handler(IRQ_PHY_TIMER_EL2, timer_handler))
    {
        LOG(ERROR, "EL2 PHY timer interrupt register failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    val_irq_enable(IRQ_PHY_TIMER_EL2, 0);

    cnt_val2 = val_read_cntpct_el0();
    cnt_frq = val_read_cntfrq_el0();

    /* Calculate the boot time of the platform */
    boot_time_ms = ((cnt_val2 - cnt_val1) * 1000) / cnt_frq;

    val_timer_set_phy_el2(boot_time_ms * 1000000);

    rec_enter->gprs[1] = boot_time_ms;

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto free_irq;
    }

    /* Check that REC exit was due to IRQ */
    if (rec_exit->exit_reason != RMI_EXIT_IRQ) {
        LOG(ERROR, "Unexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto free_irq;
    }

    if (handler_flag == 1)
    {
        LOG(ALWAYS, "EL2 PHY Interrupt triggered\n");
    } else {
        LOG(ERROR, "EL2 PHY interrupt not triggered\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto free_irq;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

free_irq:
    val_irq_disable(IRQ_PHY_TIMER_EL2);

    if (val_irq_unregister_handler(IRQ_PHY_TIMER_EL2))
    {
        LOG(ERROR, "EL2 PHY interrupt unregister failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
    }

    /* Free test resources */
destroy_realm:
    return;
}
