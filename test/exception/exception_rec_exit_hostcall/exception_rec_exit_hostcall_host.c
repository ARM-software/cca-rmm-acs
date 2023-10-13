 /*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "exception_common_host.h"

#define REALM_GPRS_DATA 0xAABBCCDD
#define HOST_GPRS_DATA 0x11223344

void exception_rec_exit_hostcall_host(void)
{
    val_host_realm_ts realm;
    uint32_t index = 0;
    uint64_t ret = 0;
    val_host_rec_exit_ts *rec_exit = NULL;
    val_host_rec_entry_ts *rec_entry = NULL;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    /* Populate realm with one RECs*/
    if (val_host_realm_setup(&realm, 1))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    rec_exit =  &(((val_host_rec_run_ts *)realm.run[0])->exit);
    rec_entry = &(((val_host_rec_run_ts *)realm.run[0])->entry);
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    /* check for rec_exit exit_reason and imm values */
    if (((rec_exit->exit_reason) != RMI_EXIT_HOST_CALL) ||
            (rec_exit->imm != VAL_SWITCH_TO_HOST))
    {
        LOG(ERROR, "\tRec Exit due to hostcall params mismatch, exit_reason %lx imm %lx\n",
                            rec_exit->exit_reason, rec_exit->imm);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    /* Compare the rec_exit and hostcall GPRS values[0-30] */
    for (index = 0; index < 31; index++)
    {
        if (rec_exit->gprs[index] != REALM_GPRS_DATA)
        {
            LOG(ERROR, "\tGPRS values mismatch: gprs[%d]= %lx\n", index, rec_exit->gprs[index]);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto destroy_realm;
        }
    }

    /* All other exit fields except for exit.givc3_*, exit_cnt* and exit.pmu_ovf_status are zero */
    if (rec_exit->esr || rec_exit->far || rec_exit->hpfar ||
        rec_exit->ripas_base || rec_exit->ripas_top || rec_exit->ripas_value)
    {
        LOG(ERROR, "\tRec_exit due to hostcall MBZ fields mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    /* Before rec enter fill the gprs values and compare the gprs values from realm side */
    for (index = 0; index < VAL_REC_EXIT_GPRS; index++)
    {
        rec_entry->gprs[index] = HOST_GPRS_DATA;
    }

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));
    LOG(ALWAYS, "\tRec enter HostCall verified\n", 0, 0);

destroy_realm:
    return;
}
