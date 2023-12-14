 /*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "exception_common_host.h"

void exception_rec_exit_psci_host(void)
{
    val_host_realm_ts realm = {0,};
    uint64_t ret = 0;
    val_host_rec_exit_ts *rec_exit = NULL;
    val_host_rec_enter_ts *rec_enter = NULL;
    uint64_t ec = 0;
    uint64_t imm = 0;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    realm.rec_count = 2;

    /* Populate realm with one RECs*/
    if (val_host_realm_setup(&realm, 1))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    rec_exit =  &(((val_host_rec_run_ts *)realm.run[0])->exit);
    rec_enter = &(((val_host_rec_run_ts *)realm.run[0])->enter);

   /* REC enter for the first time*/
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    /* extract the ec and imm values from the esr */
    exception_get_ec_imm(rec_exit->esr, &ec, &imm);
    /* check for esr_el2 for the rec exit dueto normal hostcall*/
    if (
            ((rec_exit->exit_reason) != RMI_EXIT_PSCI) ||
            /* gprs[0] holds the function is of psci call*/
            ((rec_exit->gprs[0]) != PSCI_AFFINITY_INFO_AARCH64) ||
             /* gprs[1] holds the mpidr of the target rec*/
            ((rec_exit->gprs[1]) != 1)
    )
    {
        LOG(ERROR, "\tRec Exit not dueto  psci, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }
    else
    {
        /* corrupting the gpr value*/
        rec_enter->gprs[5] = 0;
        exception_copy_exit_to_entry(rec_enter, rec_exit);
        LOG(TEST, "\tRec Exit is dueto  psci, ret=%x\n", ret, 0);
        /* if gprs[1] holds the mpidr value then need to send the psci complete message*/
        if (
            (rec_exit->gprs[1] == 1)
        )
        {
            /* Complete pending PSCI */
            ret = val_host_rmi_psci_complete(realm.rec[0], realm.rec[1], PSCI_E_SUCCESS);
            if (ret)
            {
                LOG(ERROR, "\tRMMI PSCI COMPLETE Failed, ret=%x\n", ret, 0);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
                goto destroy_realm;
            }
            LOG(TEST, "\tRMI_PSCI_COMPLETE Success, ret=%x\n", ret, 0);
        } /* if ((rec_exit->gprs[1] == 1)) */

        /* Resume back REC[0] execution */
        ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
        if (ret)
        {
            LOG(ERROR, "\tRec enter(after psci complete) failed, ret=%x\n", ret, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto destroy_realm;
        }
        LOG(TEST, "\tPSCI_AFFINITY_INFO verified, ret=%x\n", ret, 0);
    }

    /* Check that REC exit was due to PSCI_CPU_ON */
    if (val_host_check_realm_exit_psci((val_host_rec_run_ts *)realm.run[0],
                                PSCI_CPU_ON_AARCH64))
    {
        LOG(ERROR, "\tREC exit not due to PSCI_CPU_ON\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    /* Deny PSCI Request by returning PSCI_DENIED*/
    ret = val_host_rmi_psci_complete(realm.rec[0], realm.rec[1], PSCI_E_DENIED);
    if (ret)
    {
        LOG(ERROR, "\t PSCI_COMPLETE Failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    /* Enter REC[0]*/
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    LOG(TEST, "\tHost status for PSCI_CPU_ON verified, ret=%x\n", ret, 0);

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_realm:
    return;
}
