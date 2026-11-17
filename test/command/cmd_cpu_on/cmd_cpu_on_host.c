/*
 * Copyright (c) 2023, 2025-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "command_common_host.h"

#define MPIDR_RUNNABLE      1UL
#define MPIDR_NOT_RUNNABLE  2UL
#define CONTEXT_ID          0x5555UL

static val_host_realm_ts realm;

static uint32_t check_cpu_on_exit(val_host_rec_run_ts *run, uint64_t target_cpu)
{
    if (val_host_check_realm_exit_psci(run, PSCI_CPU_ON_AARCH64) ||
        (run->exit.gprs[1] != target_cpu) ||
        (run->exit.gprs[2] != VAL_PLANE0_IMAGE_BASE_IPA) ||
        (run->exit.gprs[3] != CONTEXT_ID))
        return VAL_ERROR;

    return VAL_SUCCESS;
}

void cmd_cpu_on_host(void)
{
    uint64_t ret;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    realm.rec_count = 3;

    /* Populate Realm with three RECs. */
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
    /* Complete the PSCI_CPU_ON request to create a runnable REC. */

    /* Check that REC exit was due to PSCI_CPU_ON */
    if (check_cpu_on_exit((val_host_rec_run_ts *)realm.run[0], MPIDR_RUNNABLE))
    {
        LOG(ERROR, "REC exit not due to PSCI_CPU_ON\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    ret = val_host_rmi_psci_complete(realm.rec[0], PSCI_E_SUCCESS);
    if (ret)
    {
        LOG(ERROR, " PSCI_COMPLETE Failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    /* Enter REC[0] to issue CPU_ON for the now-runnable REC[1]. */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    /* Complete the request so RMM can return PSCI_ALREADY_ON to REC[0]. */
    if (check_cpu_on_exit((val_host_rec_run_ts *)realm.run[0], MPIDR_RUNNABLE))
    {
        LOG(ERROR, "Unexpected CPU_ON request for runnable REC\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    ret = val_host_rmi_psci_complete(realm.rec[0], PSCI_E_SUCCESS);
    if (ret)
    {
        LOG(ERROR, " PSCI_COMPLETE Failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    /* Re-enter REC[0] to observe ALREADY_ON and issue CPU_ON for REC[2]. */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }

    /*************************** REC-EXIT ****************************/

    /* The positive CPU_ON request for REC[2] must also be forwarded. */
    if (check_cpu_on_exit((val_host_rec_run_ts *)realm.run[0], MPIDR_NOT_RUNNABLE))
    {
        LOG(ERROR, "Unexpected CPU_ON request for REC[2]\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_realm;
    }

    ret = val_host_rmi_psci_complete(realm.rec[0], PSCI_E_SUCCESS);
    if (ret)
    {
        LOG(ERROR, " PSCI_COMPLETE Failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto destroy_realm;
    }

    /* Enter REC[2] to verify the requested entry point was installed. */
    ret = val_host_rmi_rec_enter(realm.rec[2], realm.run[2]);
    if (ret)
    {
        LOG(ERROR, "Target REC enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_realm;
    }

    if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[2]))
    {
        LOG(ERROR, "Target REC did not execute from the requested entry point\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto destroy_realm;
    }

    /* Enter REC[0] to issue CPU_ON again for the now-runnable REC[2]. */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto destroy_realm;
    }

    /* Complete the request so RMM can return PSCI_ALREADY_ON to REC[0]. */
    if (check_cpu_on_exit((val_host_rec_run_ts *)realm.run[0], MPIDR_NOT_RUNNABLE))
    {
        LOG(ERROR, "Unexpected repeated CPU_ON request for REC[2]\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(14)));
        goto destroy_realm;
    }

    ret = val_host_rmi_psci_complete(realm.rec[0], PSCI_E_SUCCESS);
    if (ret)
    {
        LOG(ERROR, " PSCI_COMPLETE Failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(15)));
        goto destroy_realm;
    }

    /* Re-enter REC[0] to observe ALREADY_ON and finish the Realm checks. */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(16)));
        goto destroy_realm;
    }

    if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "Realm did not complete CPU_ON checks\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(17)));
        goto destroy_realm;
    }

    /* Free test resources */
destroy_realm:
    return;
}
