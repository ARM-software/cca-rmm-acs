/*
 * Copyright (c) 2023, 2025-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "command_common_host.h"

static val_host_realm_ts realm;

void cmd_affinity_info_host(void)
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

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    /* Check that REC exit was due to PSCI_CPU_ON */
    if (val_host_check_realm_exit_psci((val_host_rec_run_ts *)realm.run[0],
                                PSCI_CPU_ON_AARCH64))
    {
        LOG(ERROR, "REC exit not due to PSCI_CPU_ON\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    /* The target REC is identified from the pending CPU_ON request. */
    ret = val_host_rmi_psci_complete(realm.rec[0], PSCI_E_SUCCESS);
    if (ret)
    {
        LOG(ERROR, " PSCI_COMPLETE Failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    /* Enter REC[0] */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    /* AFFINITY_INFO is handled by RMM and must not cause a PSCI exit. */
    if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "Realm did not complete AFFINITY_INFO checks\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    /* Free test resources */
destroy_realm:
    return;
}
