/*
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "mm_common_host.h"
#include "val_host_helpers.h"

#define PROTECTED_IPA 0x800000

void mm_hipas_unassigned_ripas_ram_da_ia_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;
    val_host_rec_enter_ts *rec_enter = NULL;
    val_host_rec_exit_ts *rec_exit = NULL;
    val_data_create_ts data_create;
    uint64_t da_ipa, ia_ipa, ripas_size;
    uint64_t phys;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm, false))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    data_create.size = 2 * PAGE_SIZE;
    phys = (uint64_t)val_host_mem_alloc(PAGE_SIZE, data_create.size);
    if (!phys)
    {
        LOG(ERROR, "\tval_host_mem_alloc failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    data_create.target_pa = phys;
    data_create.ipa = PROTECTED_IPA;
    data_create.rtt_alignment = PAGE_SIZE;
    ret = val_host_init_ripas_delegate(&realm, &data_create);
    if (ret)
    {
        LOG(ERROR, "\tval_host_init_ripas_delegate failed\n", 0, 0);
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

    rec_enter = &(((val_host_rec_run_ts *)realm.run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);
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

    /* Resume back REC[0] execution */
    da_ipa = PROTECTED_IPA;
    ia_ipa = PROTECTED_IPA + PAGE_SIZE;
    ripas_size = data_create.size;
    rec_enter->gprs[1] = da_ipa;
    rec_enter->gprs[2] = ia_ipa;
    rec_enter->gprs[3] = ripas_size;
    /* Test intent: Protected IPA, HIPAS=UNASSIGNED, RIPAS=RAM data access
     * => REC exit due to data abort
     */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    if (validate_rec_exit_da(rec_exit, da_ipa, ESR_ISS_DFSC_TTF_L3,
                                NON_EMULATABLE_DA, ESR_WnR_WRITE))
    {
        LOG(ERROR, "\tREC exit DA: params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }

    /* Fix the Data Abort */
    phys = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!phys)
    {
        LOG(ERROR, "\tval_host_mem_alloc failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_realm;
    }

    ret = val_host_map_protected_data_unknown(&realm, phys, da_ipa, PAGE_SIZE);
    if (ret)
    {
        LOG(ERROR, "\tDATA_CREATE_UNKNOWN failed, ret = %d \n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto destroy_realm;
    }

    /* Test intent: Protected IPA, HIPAS=UNASSIGNED, RIPAS=RAM instruction access
     * => REC exit due to instruction abort
     */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_realm;
    }

    if (validate_rec_exit_ia(rec_exit, ia_ipa))
    {
        LOG(ERROR, "\tREC exit IA: params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
