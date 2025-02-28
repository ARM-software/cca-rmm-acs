/*
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"

#define MEC_SHARED_ID 0x1
#define MEC_PRIVATE_ID_1 0x2
#define MEC_PRIVATE_ID_2 0x3

void mec_private_shared_host(void)
{
    val_host_realm_ts realm1;
    val_host_realm_ts realm2;
    val_host_realm_ts realm3;
    val_host_realm_ts realm4;
    uint64_t ret;
    val_host_rec_exit_ts *rec_exit = NULL;
    val_smc_param_ts args;
    uint64_t featreg1;

    val_host_rmi_features(1, &featreg1);
    if (!featreg1)
    {
        LOG(ERROR, "MEC feature not supported, skipping the test\n", 0, 0);
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto destroy_realm;
    }

    val_memset(&realm1, 0, sizeof(realm1));
    val_memset(&realm2, 0, sizeof(realm2));
    val_memset(&realm3, 0, sizeof(realm3));
    val_memset(&realm4, 0, sizeof(realm4));

    val_host_realm_params(&realm1);
    val_host_realm_params(&realm2);
    val_host_realm_params(&realm3);
    val_host_realm_params(&realm4);

    realm1.mecid = MEC_SHARED_ID;
    args = val_host_rmi_mec_set_shared(realm1.mecid);
    if (args.x0)
    {
        LOG(ERROR, "\trmi_mec_set_shared failed %x\n", args.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    /* Populate realm-1 with one REC*/
    if (val_host_realm_setup(&realm1, 1))
    {
        LOG(ERROR, "\tRealm-1 setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }
    LOG(DBG, "\tCreated Realm-1 with Shared MECID: %d\n", realm1.mecid, 0);

    realm2.vmid = 1;
    realm2.mecid = realm1.mecid;
    /* Populate realm-2 with one REC*/
    if (val_host_realm_setup(&realm2, 1))
    {
        LOG(ERROR, "\tRealm-2 setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }
    LOG(DBG, "\tCreated Realm-2 with Shared MECID: %d\n", realm2.mecid, 0);

    realm3.vmid = 2;
    realm3.mecid = MEC_PRIVATE_ID_1;
    /* Populate realm-3 with one REC*/
    if (val_host_realm_setup(&realm3, 1))
    {
        LOG(ERROR, "\tRealm-3 setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }
    LOG(DBG, "\tCreated Realm-3 with Private MECID: %d\n", realm3.mecid, 0);

    realm4.vmid = 3;
    realm4.mecid = MEC_PRIVATE_ID_2;
    /* Populate realm-4 with one REC*/
    if (val_host_realm_setup(&realm4, 1))
    {
        LOG(ERROR, "\tRealm-4 setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }
    LOG(DBG, "\tCreated Realm-4 with Private MECID: %d\n", realm4.mecid, 0);


    /* Enter Realm-1 REC[0]  */
    ret = val_host_rmi_rec_enter(realm1.rec[0], realm1.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRealm-1 Rec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    rec_exit = &(((val_host_rec_run_ts *)realm1.run[0])->exit);
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    /* Enter Realm-2 REC[0]  */
    ret = val_host_rmi_rec_enter(realm2.rec[0], realm2.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRealm-2 Rec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    rec_exit = &(((val_host_rec_run_ts *)realm2.run[0])->exit);
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    /* Enter Realm-3 REC[0]  */
    ret = val_host_rmi_rec_enter(realm3.rec[0], realm3.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRealm-2 Rec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    rec_exit = &(((val_host_rec_run_ts *)realm3.run[0])->exit);
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    /* Enter Realm-4 REC[0]  */
    ret = val_host_rmi_rec_enter(realm4.rec[0], realm4.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRealm-4 Rec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    rec_exit = &(((val_host_rec_run_ts *)realm4.run[0])->exit);
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    if (val_host_postamble())
    {
        LOG(ERROR, "\tval_host_postamble failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR));
    }

    args = val_host_rmi_mec_set_private(MEC_SHARED_ID);
    if (args.x0)
    {
        LOG(ERROR, "\trmi_mec_set_private failed %x\n", args.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR));
    }

    return;
}
