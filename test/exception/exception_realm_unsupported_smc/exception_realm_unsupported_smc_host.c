 /*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "exception_common_host.h"

void exception_realm_unsupported_smc_host(void)
{
    val_host_realm_ts realm = {0,};
    uint64_t ret = 0;
    val_host_rec_exit_ts *rec_exit = NULL;
    uint64_t ec = 0;
    uint64_t imm = 0;

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

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    /* extract the ec and imm values from the esr */
    exception_get_ec_imm(rec_exit->esr, &ec, &imm);
    /* check for esr_el2 for the rec exit dueto normal hostcall*/
    if (
            ((rec_exit->exit_reason) != RMI_EXIT_HOST_CALL) &&
            (imm != VAL_SWITCH_TO_HOST)
       )
    {
        LOG(ERROR, "\tRec Exit not dueto  hostcall, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    } else
    {
        val_set_status(RESULT_PASS(VAL_SUCCESS));
    }

destroy_realm:
    return;
}
