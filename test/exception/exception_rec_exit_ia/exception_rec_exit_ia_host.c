 /*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "exception_common_host.h"
#include "val_host_rmi.h"
#include "val_host_helpers.h"

#define MAP_LEVEL_FOR_WALK 3
#define EXCEPTION_IA_IPA_ADDRESS 0x501000

void exception_rec_exit_ia_host(void)
{
    val_host_realm_ts realm = {0,};
    uint64_t ret = 0;
    val_host_rec_entry_ts *rec_entry = NULL;
    val_host_rec_exit_ts *rec_exit = NULL;

    val_host_realm_params(&realm);

    /* Populate realm with one RECs*/
    if (val_host_realm_setup(&realm, 0))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    if (val_host_ripas_init(&realm, EXCEPTION_IA_IPA_ADDRESS, EXCEPTION_IA_IPA_ADDRESS + PAGE_SIZE,
                                                                     VAL_RTT_MAX_LEVEL, PAGE_SIZE))
    {
        LOG(ERROR, "\trealm_init_ipa_state failed, ipa=0x%x\n",
                EXCEPTION_IA_IPA_ADDRESS, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    /* Activate realm */
    if (val_host_realm_activate(&realm))
    {
        LOG(ERROR, "\tRealm activate failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    rec_entry = &(((val_host_rec_run_ts *)realm.run[0])->entry);
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);
    /* Enter the realm through rec enter*/
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    /* pass the address to realm */
    rec_entry->gprs[1] = EXCEPTION_IA_IPA_ADDRESS;

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    if (validate_rec_exit_ia(rec_exit, EXCEPTION_IA_IPA_ADDRESS))
    {
        LOG(ERROR, "\tREC exit params mismatch exit_reason %lx esr %lx\n",
                            rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));
destroy_realm:
    return;
}
