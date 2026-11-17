/*
 * Copyright (c) 2023, 2025-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_sysreg.h"

#define ID_AA64MMFR3_EL1_S2PIE_SHIFT    12U
#define ID_AA64MMFR3_EL1_S2PIE_MASK     0xFU

void cmd_rsi_features_host(void)
{
    val_host_realm_ts realm;
    val_host_rec_enter_ts *rec_enter;
    val_host_rec_exit_ts *rec_exit;
    uint64_t ret;
    uint64_t s2pie;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm, 1))
    {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    rec_enter = &(((val_host_rec_run_ts *)realm.run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);

    /* Enter REC[0] to receive the feature query request. */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL)
    {
        LOG(ERROR, "Unexpected REC exit, reason=%u\n", rec_exit->exit_reason);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    s2pie = (val_id_aa64mmfr3_el1_read() >> ID_AA64MMFR3_EL1_S2PIE_SHIFT) &
                                                    ID_AA64MMFR3_EL1_S2PIE_MASK;
    rec_enter->gprs[1] = (s2pie != 0U) ? 1U : 0U;

    /* Return the expected MRO value and allow the Realm test to complete. */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    /* Free test resources */
destroy_realm:
    return;
}
