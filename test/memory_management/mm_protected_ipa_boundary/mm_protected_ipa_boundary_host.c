/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "mm_common_host.h"
#include "val_host_helpers.h"

void mm_protected_ipa_boundary_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;
    val_host_rec_entry_ts *rec_entry = NULL;
    val_host_rec_exit_ts *rec_exit = NULL;
    uint64_t protected_ipa, unprotected_ipa;
    uint64_t protected_src_pa, protected_target_pa;
    uint64_t unprotected_pa;
    val_host_data_destroy_ts data_destroy;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm, false))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    protected_ipa = (1UL << (realm.s2sz - 1)) - PAGE_SIZE;
    unprotected_ipa = (1UL << (realm.s2sz - 1));

    if (val_host_ripas_init(&realm, protected_ipa, protected_ipa + PAGE_SIZE,
                                               VAL_RTT_MAX_LEVEL, PAGE_SIZE))
    {
        LOG(ERROR, "\trealm_init_ipa_state failed, ipa=0x%lx\n",
                protected_ipa, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    protected_src_pa = (uint64_t)val_host_mem_alloc(PAGE_SIZE, (2*PAGE_SIZE));
    if (!protected_src_pa)
    {
        LOG(ERROR, "\tval_host_mem_alloc failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }
    protected_target_pa = protected_src_pa + PAGE_SIZE;
    ret = val_host_map_protected_data(&realm, protected_target_pa,
                                    protected_ipa, PAGE_SIZE, protected_src_pa);
    if (ret)
    {
        LOG(ERROR, "\tMap protect data failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    /* Activate realm */
    if (val_host_realm_activate(&realm))
    {
        LOG(ERROR, "\tRealm activate failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    unprotected_pa = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!unprotected_pa)
    {
        LOG(ERROR, "\tval_host_mem_alloc failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    if (val_host_map_unprotected(&realm, unprotected_pa, unprotected_ipa, PAGE_SIZE, PAGE_SIZE))
    {
        LOG(ERROR, "\tval_realm_map_unprotected_data failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    rec_entry = &(((val_host_rec_run_ts *)realm.run[0])->entry);
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);
    /* REC enter REC[0] execution */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    } else if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "\tREC_EXIT: HOST_CALL params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_realm;
    }

    ret = val_host_rmi_data_destroy(realm.rd, protected_ipa, &data_destroy);
    if (ret)
    {
        LOG(ERROR, "\tData destroy failed, ipa=0x%lx, ret=0x%x\n", protected_ipa, ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto destroy_realm;
    }

    /* Resume back REC[0] execution */
    rec_entry->gprs[1] = protected_ipa;
    /* Test intent: Protected IPA boundary check */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_realm;
    }

    if (validate_rec_exit_da(rec_exit, protected_ipa, ESR_ISS_DFSC_TTF_L3,
                                NON_EMULATABLE_DA, 0))
    {
        LOG(ERROR, "\tREC exit params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
