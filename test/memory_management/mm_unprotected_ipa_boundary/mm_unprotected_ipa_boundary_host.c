/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"

void mm_unprotected_ipa_boundary_host(void)
{
    val_host_realm_ts realm;
    val_host_rec_entry_ts *rec_entry = NULL;
    val_host_rec_exit_ts *rec_exit = NULL;
    uint64_t ret;
    uint64_t protected_ipa, unprotected_ipa;
    uint64_t protected_src_pa, protected_target_pa;
    uint64_t unprotected_pa;
    uint64_t top;

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

    ret = val_host_rmi_rtt_unmap_unprotected(realm.rd, unprotected_ipa, VAL_RTT_MAX_LEVEL, &top);
    if (ret)
    {
        LOG(ERROR, "\tval_rmi_rtt_unmap_unprotected failed, ipa=0x%x, ret=0x%x\n",
                                                             unprotected_ipa, ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto destroy_realm;
    }

    rec_entry = &(((val_host_rec_run_ts *)realm.run[0])->entry);
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);
    rec_entry->gprs[1] = protected_ipa;
    rec_entry->gprs[2] = unprotected_ipa;
    rec_entry->flags = 0x0;
    /* Test Intent: UnProtected IPA boundary check */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_realm;
    }

    if (!((rec_exit->exit_reason == RMI_EXIT_SYNC) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 11, 12) == ESR_ISS_SET_UER) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 9, 9) == ESR_ISS_EA) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 0, 5) == ESR_ISS_DFSC_TTF_L3) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 26, 31) == ESR_EC_LOWER_EL) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 24, 24) == ESR_ISV_VALID) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 22, 23) == ESR_SAS_WORD) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 6, 6) == ESR_WnR_WRITE) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 10, 10) == ESR_FnV) &&
        (VAL_EXTRACT_BITS(unprotected_ipa, 8, 63) == rec_exit->hpfar)))
    {
        LOG(ERROR, "\tREC exit params mismatch: esr %lx\n", rec_exit->esr, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
