/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
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

void planes_rec_exit_irq_host(void)
{
    static val_host_realm_ts realm;
    val_host_realm_flags1_ts realm_flags;
    uint64_t ret;
    val_smc_param_ts cmd_ret;
    val_host_rec_exit_ts *rec_exit = NULL;
    val_host_rec_enter_ts *rec_enter = NULL;
    uint64_t s2ap_ipa_base, s2ap_ipa_top;
    uint64_t cnt_val1, cnt_val2, cnt_frq, boot_time_ms;

    /* Skip if RMM do not support planes */
    if (!val_host_rmm_supports_planes())
    {
        LOG(ALWAYS, "\n\tPlanes feature not supported\n", 0, 0);
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

    LOG(DBG, "\t INFO: RTT tree per plane : %d\n", realm_flags.rtt_tree_pp, 0);

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm, 1))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
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
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    /* Check that REC exit was due to S2AP change request */
    if (rec_exit->exit_reason != RMI_EXIT_S2AP_CHANGE) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    s2ap_ipa_base = rec_exit->s2ap_base;
    s2ap_ipa_top =  rec_exit->s2ap_top;

    while (s2ap_ipa_base != s2ap_ipa_top) {
        cmd_ret = val_host_rmi_rtt_set_s2ap(realm.rd, realm.rec[0], s2ap_ipa_base, s2ap_ipa_top);
        if (cmd_ret.x0) {
            LOG(ERROR, "\nRMI_SET_S2AP failed with ret= 0x%x\n", cmd_ret.x0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto destroy_realm;
        }
        s2ap_ipa_base = cmd_ret.x1;
    }

    rec_enter->flags = 0x0;

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    /* Check that REC exit was due to host call for P0 and P1 is initialized */
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    /* Initialize EL2 timer interrupt after P0 and P1 is booted and initialized */
    if (val_irq_register_handler(IRQ_PHY_TIMER_EL2, timer_handler))
    {
        LOG(ERROR, "\tEL2 PHY timer interrupt register failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
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
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto free_irq;
    }

    /* Check that REC exit was due to IRQ */
    if (rec_exit->exit_reason != RMI_EXIT_IRQ) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto free_irq;
    }

    if (handler_flag == 1)
    {
        LOG(ALWAYS, "\tEL2 PHY Interrupt triggered\n", 0, 0);
    } else {
        LOG(ERROR, "\tEL2 PHY interrupt not triggered\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto free_irq;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

free_irq:
    val_irq_disable(IRQ_PHY_TIMER_EL2);

    if (val_irq_unregister_handler(IRQ_PHY_TIMER_EL2))
    {
        LOG(ERROR, "\tEL2 PHY interrupt unregister failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
    }

    /* Free test resources */
destroy_realm:
    return;
}
