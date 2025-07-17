/*
 *
 * Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_helpers.h"
#include "val_timer.h"
#include "planes_timer_state_rec_exit_data.h"

void planes_timer_state_rec_exit_host(void)
{
    static val_host_realm_ts realm;
    val_host_realm_flags1_ts realm_flags;
    uint64_t ret, i;
    val_host_rec_exit_ts *rec_exit = NULL;

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
    if (val_host_realm_setup(&realm, true))
    {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

   /* Check that REC exit was due S2AP change request */
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

    for (i = 0; i < (sizeof(test_data) / sizeof(test_stimulus)); i++)
    {

        /* Timer interrupts while executing in Pn causes REC exit due to IRQ */
        if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL)
        {
            LOG(ERROR, "Rec exit params mismatch, exit_reason=%x esr %lx\n",
                            rec_exit->exit_reason, rec_exit->cntp_ctl);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
            goto destroy_realm;
        }

        if (rec_exit->cntp_cval != rec_exit->gprs[test_data[i].exp_plane_idx])
        {
            LOG(ERROR, "Unexpected Timer value %x. Expected %lx\n", rec_exit->cntp_cval,
                                                     rec_exit->gprs[test_data[i].exp_plane_idx]);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
            goto destroy_realm;

        }

        /* Enter REC[0]  */
        ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
        if (ret)
        {
            LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto destroy_realm;
        }


    }

    /* Check that REC exit is due to HOST CALL after completing of test */
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL)
    {
        LOG(ERROR, "Rec exit params mismatch, exit_reason=%x esr %lx\n",
                        rec_exit->exit_reason, rec_exit->cntp_ctl);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
