/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_irq.h"

void exception_rec_exit_fiq_host(void)
{
#ifndef SECURE_TEST_ENABLE
    val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
    return;
#else
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

    /* Execute secure payload  */
    if (val_host_execute_secure_payload())
    {
        LOG(ERROR, "\tval_execute_secure_payload() failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    if ((rec_exit->exit_reason != RMI_EXIT_FIQ) || rec_exit->esr)
    {
        LOG(ERROR, "\tRec exit params mismatch, exit_reason=%x esr %lx\n",
                        rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
#endif
}
