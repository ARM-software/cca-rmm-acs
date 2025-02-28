/*
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"


void mec_id_private_host(void)
{
    val_host_realm_ts realm1;
    uint64_t ret;
    val_host_rec_exit_ts *rec_exit = NULL;
    uint64_t featreg1;

    val_host_rmi_features(1, &featreg1);
    if (!featreg1)
    {
        LOG(ERROR, "MEC feature not supported, skipping the test\n", 0, 0);
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto destroy_realm;
    }

    val_memset(&realm1, 0, sizeof(realm1));

    val_host_realm_params(&realm1);

    realm1.mecid = 1;

    /* Populate realm-1 with Private MECID */
    if (val_host_realm_setup(&realm1, 1))
    {
        LOG(ERROR, "\tRealm-1 setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    LOG(DBG, "\tCreated Realm-1 with Private MECID: %d\n", realm1.mecid, 0);

    /* Enter Realm-1 REC[0]  */
    ret = val_host_rmi_rec_enter(realm1.rec[0], realm1.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRealm-1 Rec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    rec_exit = &(((val_host_rec_run_ts *)realm1.run[0])->exit);
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
