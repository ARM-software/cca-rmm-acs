/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "exception_common_host.h"
#include "val_host_rmi.h"

#define PROTECTED_IPA 0x800000

void exception_rec_exit_ripas_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret = 0;
    val_host_rec_entry_ts *rec_entry = NULL;
    uint64_t ripas_ipa = 0, ripas_size = 0x3000;
    exception_rec_exit_ts exception_rec_exit = {0,};
    uint64_t phys;
    val_data_create_ts data_create;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm, false))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    data_create.size = ripas_size;
    phys = (uint64_t)val_host_mem_alloc(PAGE_SIZE, (2 * data_create.size));
    if (!phys)
    {
        LOG(ERROR, "\tval_host_mem_alloc failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    data_create.src_pa = phys;
    data_create.target_pa = phys + data_create.size;
    data_create.ipa = PROTECTED_IPA;
    data_create.rtt_alignment = PAGE_SIZE;
    ret = val_host_map_protected_data_to_realm(&realm, &data_create);
    if (ret)
    {
        LOG(ERROR, "\tval_host_map_protected_data_to_realm failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    /* Activate realm */
    if (val_host_realm_activate(&realm))
    {
        LOG(ERROR, "\tRealm activate failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    rec_entry = &(((val_host_rec_run_ts *)realm.run[0])->entry);
    /* REC enter REC[0] execution */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    } else if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "\tREC_EXIT: HOST_CALL params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    /*Step 1: RAM --> EMPTY*/
    ripas_ipa = PROTECTED_IPA;
    rec_entry->gprs[1] = ripas_ipa;
    rec_entry->gprs[2] = ripas_size;
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    } else if (val_host_check_realm_exit_ripas_change((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "\tRipas change req failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }
    exception_rec_exit.ripas_base = ripas_ipa;
    exception_rec_exit.ripas_top = ripas_ipa + ripas_size;
    exception_rec_exit.ripas_value = RSI_EMPTY;
    exception_rec_exit.exception_ripas_exit_intent = EXCEPTION_RIPAS_EXIT_ACCEPT;
    exception_rec_exit.realm = &realm;
    ret = exception_validate_rec_exit_ripas(exception_rec_exit);
    if (ret)
    {
        LOG(ERROR, "\trec exit validation failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_realm;
    }
    /* rec enter for letting the realm know the service request is finished*/
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto destroy_realm;
    } else if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "\tREC_EXIT: HOST_CALL params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_realm;
    }

    /*Step 2: EMPTY --> RAM*/
    ripas_ipa = PROTECTED_IPA;
    rec_entry->gprs[1] = ripas_ipa;
    rec_entry->gprs[2] = ripas_size;
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto destroy_realm;
    } else if (val_host_check_realm_exit_ripas_change((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "\tRipas change req failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto destroy_realm;
    }
    exception_rec_exit.ripas_base = ripas_ipa;
    exception_rec_exit.ripas_top = ripas_ipa + ripas_size;
    exception_rec_exit.ripas_value = RSI_RAM;
    exception_rec_exit.exception_ripas_exit_intent = EXCEPTION_RIPAS_EXIT_ACCEPT;
    exception_rec_exit.realm = &realm;
    ret = exception_validate_rec_exit_ripas(exception_rec_exit);
    if (ret)
    {
        LOG(ERROR, "\trec exit validation failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(14)));
        goto destroy_realm;
    }
    /* rec enter for letting the realm know the service request is finished*/
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(15)));
        goto destroy_realm;
    } else if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "\tREC_EXIT: HOST_CALL params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(16)));
        goto destroy_realm;
    }

    /*Step 3: RAM --> EMPTY */
    ripas_ipa = PROTECTED_IPA;
    rec_entry->gprs[1] = ripas_ipa;
    rec_entry->gprs[2] = ripas_size;
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(17)));
        goto destroy_realm;
    } else if (val_host_check_realm_exit_ripas_change((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "\tRipas change req failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(18)));
        goto destroy_realm;
    }
    exception_rec_exit.ripas_base = ripas_ipa;
    exception_rec_exit.ripas_top = ripas_ipa + ripas_size;
    exception_rec_exit.ripas_value = RSI_EMPTY;
    exception_rec_exit.exception_ripas_exit_intent = EXCEPTION_RIPAS_EXIT_PARTIAL;
    exception_rec_exit.realm = &realm;
    ret = exception_validate_rec_exit_ripas(exception_rec_exit);
    if (ret)
    {
        LOG(ERROR, "\trec exit validation failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(19)));
        goto destroy_realm;
    }
    /* rec enter for letting the realm know the service request is finished*/
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(20)));
        goto destroy_realm;
    } else if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "\tREC_EXIT: HOST_CALL params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(21)));
        goto destroy_realm;
    }

    /*Step 4: RAM --> EMPTY */
    ripas_ipa = PROTECTED_IPA;
    rec_entry->gprs[1] = ripas_ipa + 0x1000;
    rec_entry->gprs[2] = 0x1000;
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(22)));
        goto destroy_realm;
    } else if (val_host_check_realm_exit_ripas_change((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "\tRipas change req failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(23)));
        goto destroy_realm;
    }
    exception_rec_exit.ripas_base = ripas_ipa + 0x1000;
    exception_rec_exit.ripas_top = ripas_ipa + 0x2000;
    exception_rec_exit.ripas_value = RSI_EMPTY;
    exception_rec_exit.exception_ripas_exit_intent = EXCEPTION_RIPAS_EXIT_REJECT;
    exception_rec_exit.realm = &realm;
    ret = exception_validate_rec_exit_ripas(exception_rec_exit);
    if (ret)
    {
        LOG(ERROR, "\trec exit validation failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(24)));
        goto destroy_realm;
    }
    /* rec enter for letting the realm know the service request is finished*/
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(25)));
        goto destroy_realm;
    } else if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "\tREC_EXIT: HOST_CALL params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(26)));
        goto destroy_realm;
    }
    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
