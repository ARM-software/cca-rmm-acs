/*
 * Copyright (c) 2023, 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "command_common_host.h"

#define REC_RUNNABLE         1
#define REC_NOT_RUNNABLE     2

static val_host_realm_ts realm;

void cmd_cpu_on_host(void)
{
    uint64_t ret;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    realm.rec_count = 3;

    /* Populate realm with two RECs*/
    if (val_host_realm_setup(&realm, 1))
    {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    /* Enter REC[0] */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    /*************************** REC-EXIT ****************************/
    /* Complet PSCI_CPU_ON request to create a RUNNABLE rec */

    /* Check that REC exit was due to PSCI_CPU_ON */
    if (val_host_check_realm_exit_psci((val_host_rec_run_ts *)realm.run[0],
                                PSCI_CPU_ON_AARCH64))
    {
        LOG(ERROR, "REC exit not due to PSCI_CPU_ON\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    /* Complete pending PSCI by returning a RUNNABLE target REC*/
    ret = val_host_rmi_psci_complete(realm.rec[0], realm.rec[REC_RUNNABLE], PSCI_E_SUCCESS);
    if (ret)
    {
        LOG(ERROR, " PSCI_COMPLETE Failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    /* Enter REC[0] for failure condition check*/
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    /*************************** REC-EXIT ****************************/

    /* Check that REC exit was due to PSCI_CPU_ON */
    if (val_host_check_realm_exit_psci((val_host_rec_run_ts *)realm.run[0],
                                PSCI_CPU_ON_AARCH64))
    {
        LOG(ERROR, "REC exit not due to PSCI_CPU_ON\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    /* Complete pending PSCI by returning a RUNNABLE target REC*/
    ret = val_host_rmi_psci_complete(realm.rec[0], realm.rec[REC_RUNNABLE], PSCI_E_SUCCESS);
    if (ret)
    {
        LOG(ERROR, " PSCI_COMPLETE Failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    /* Enter REC[0] to continue failure condition check*/
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }

    /*************************** REC-EXIT ****************************/

    /* Check that REC exit was due to PSCI_CPU_ON */
    if (val_host_check_realm_exit_psci((val_host_rec_run_ts *)realm.run[0],
                                PSCI_CPU_ON_AARCH64))
    {
        LOG(ERROR, "REC exit not due to PSCI_CPU_ON\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_realm;
    }

    /* Complete pending PSCI by returning a NOT_RUNNABLE target REC */
    ret = val_host_rmi_psci_complete(realm.rec[0], realm.rec[REC_NOT_RUNNABLE], PSCI_E_SUCCESS);
    if (ret)
    {
        LOG(ERROR, "PSCI_COMPLETE Failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto destroy_realm;
    }

    /* Enter REC[0] for positive observability check  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_realm;
    }

    /*************************** REC-EXIT ****************************/

    /* Check that REC exit was due to PSCI_CPU_ON */
    if (val_host_check_realm_exit_psci((val_host_rec_run_ts *)realm.run[0],
                                PSCI_CPU_ON_AARCH64))
    {
        LOG(ERROR, "REC exit not due to PSCI_CPU_ON\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto destroy_realm;
    }

    /* Complete pending PSCI by returning a NOT_RUNNABLE target REC */
    ret = val_host_rmi_psci_complete(realm.rec[0], realm.rec[REC_NOT_RUNNABLE], PSCI_E_SUCCESS);
    if (ret)
    {
        LOG(ERROR, "PSCI_COMPLETE Failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto destroy_realm;
    }

    /* Enter REC[0] to continue positive observability check*/
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(14)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
