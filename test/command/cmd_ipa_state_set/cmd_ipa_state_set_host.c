/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"


void cmd_ipa_state_set_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret, out_top;
    uint64_t ipa_base, ipa_top;
    val_host_rec_exit_ts *rec_exit;
    val_host_rec_enter_ts *rec_enter = NULL;
    val_host_rec_enter_flags_ts rec_enter_flags;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm, 1))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }
    else if (val_host_check_realm_exit_ripas_change((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "\tREC_EXIT: Unexpected REC exit\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    /* Complete the RIPAS change request */
    rec_exit =  &(((val_host_rec_run_ts *)realm.run[0])->exit);

    ipa_base = rec_exit->ripas_base;
    ipa_top = rec_exit->ripas_top;

    ret = val_host_rmi_rtt_set_ripas(realm.rd, realm.rec[0], ipa_base, ipa_top, &out_top);
    if  (ret)
    {
        LOG(ERROR, "\tRIPAS_CHANGE failed with ret = 0x%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;

    }

    rec_enter = &(((val_host_rec_run_ts *)realm.run[0])->enter);
    rec_enter_flags.ripas_response = RMI_ACCEPT;
    val_memcpy(&rec_enter->flags, &rec_enter_flags, sizeof(rec_enter_flags));

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    /* Reject RIPAS request */
    rec_enter_flags.ripas_response = RMI_REJECT;
    val_memcpy(&rec_enter->flags, &rec_enter_flags, sizeof(rec_enter_flags));

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
