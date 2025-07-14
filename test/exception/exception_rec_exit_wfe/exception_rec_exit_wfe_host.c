/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "exception_common_host.h"

void exception_rec_exit_wfe_host(void)
{
    val_host_realm_ts realm = {0,};
    uint64_t ret = 0;
    val_host_rec_enter_ts *rec_enter = NULL;
    val_host_rec_exit_ts *rec_exit = NULL;
    uint64_t ec = 0;
    uint64_t imm = 0;
    val_host_rec_enter_flags_ts rec_enter_flags;
    uint64_t *wfe_trig = (uint64_t *)(val_get_shared_region_base() + VAL_TEST_USE1);

    val_memset(&realm, 0, sizeof(realm));
    val_memset(&rec_enter_flags, 0, sizeof(rec_enter_flags));

    val_host_realm_params(&realm);
    *wfe_trig = false;

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Populate realm with one RECs*/
    if (val_host_realm_setup(&realm, 1))
    {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    rec_enter = &(((val_host_rec_run_ts *)realm.run[0])->enter);
    rec_exit =  &(((val_host_rec_run_ts *)realm.run[0])->exit);

    /* set the wfe trap bit in the rec_run obj */
    rec_enter_flags.trap_wfe = 1;
    rec_enter_flags.trap_wfi = 1;
    val_memcpy(&rec_enter->flags, &rec_enter_flags, sizeof(rec_enter_flags));

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    /* Upon REC exit extract the ec and imm values from the esr */
    exception_get_ec_imm(rec_exit->esr, &ec, &imm);

    /* check for esr_el2 for the rec exit due to wfe */
    if (ec != ESR_EL2_EC_WFX || ESR_EL2_WFX_TI(rec_exit->esr) != ESR_EL2_WFX_TI_WFE)
    {
        LOG(ERROR, "Rec Exit not due to  WFE, ESR=%lx\n", rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    LOG(TEST, "WFE Trigger verified \n");
    *wfe_trig = true;

    /* populate the rec exit details into the rec enter and corrupt some possible gprs
     * in the range 0-6
     */
    exception_copy_exit_to_entry(rec_enter, rec_exit);
    val_memcpy(&rec_enter->flags, &rec_enter_flags, sizeof(rec_enter_flags));
    rec_enter->gprs[5] = 0;
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret != 0)
    {
        LOG(ERROR, "Rec enter failed ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
    }

    /* Check that REC exit was due to host call from realm after completing test */
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "Unexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_realm:
    return;
}
