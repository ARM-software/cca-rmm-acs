/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_helpers.h"

#define TEST_IPA 0x1000

void planes_rec_exit_da_ia_hipas_unassigned_ripas_ram_host(void)
{
    static val_host_realm_ts realm;
    val_host_realm_flags1_ts realm_flags;
    uint64_t ret;
    val_host_rec_exit_ts *rec_exit = NULL;
    val_host_rec_enter_ts *rec_enter = NULL;
    uint64_t da_ipa, ia_ipa, size, phys;

    /* Skip if RMM do not support planes */
    if (!val_host_rmm_supports_planes())
    {
        LOG(ALWAYS, "\n\tPlanes feature not supported\n", 0, 0);
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto destroy_realm;
    }

    val_memset(&realm, 0, sizeof(realm));
    val_memset(&realm_flags, 0, sizeof(realm_flags));

    val_host_realm_params(&realm);

    /* Overwrite Realm Parameters */
    realm.num_aux_planes = 1;
    realm_flags.rtt_tree_pp = RMI_FEATURE_TRUE;

    if (val_host_rmm_supports_rtt_tree_single())
        realm_flags.rtt_tree_pp = RMI_FEATURE_FALSE;

    val_memcpy(&realm.flags1, &realm_flags, sizeof(realm.flags1));

    LOG(DBG, "\t INFO: RTT tree per plane : %d\n", realm_flags.rtt_tree_pp, 0);
    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm, false))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    da_ipa = TEST_IPA;
    ia_ipa = TEST_IPA + PAGE_SIZE;
    size = 2 * PAGE_SIZE;
    /* Prepare IPA whose HIPAS = UNASIGNED, RIPAS = RAM, the process to put the IPA
     * to this state depends on whether we are acccessing primary or auxiliary RTT tree */
    if (val_host_ripas_init(&realm, TEST_IPA, TEST_IPA + size,
                                                     VAL_RTT_MAX_LEVEL, PAGE_SIZE))
    {
            LOG(ERROR, "\tRMI_INIT_RIPAS failed ", 0, 0);
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

    rec_enter = &(((val_host_rec_run_ts *)realm.run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    /* Check that REC Exit was due to host call because of P0 requesting for test IPA */
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    /* Return the test IPA to P0 */
    rec_enter->gprs[1] = da_ipa;
    rec_enter->gprs[2] = ia_ipa;
    rec_enter->gprs[3] = size;

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    /* Check that REC exit was due S2AP change request */
    if (rec_exit->exit_reason != RMI_EXIT_S2AP_CHANGE) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    /* Update S2AP for the requested memory range */
    if (val_host_set_s2ap(&realm))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }

    /* Check that REC exit was due to Data Abort due to P1 access to IPA whose
     * HIPAS,RIPAS = UNASSIGNED,RAM */
    if (validate_rec_exit_da(rec_exit, da_ipa, ESR_ISS_DFSC_TTF_L3,
                                NON_EMULATABLE_DA, ESR_WnR_WRITE))
    {
        LOG(ERROR, "\tREC exit DA: params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_realm;
    }

    /* Fix the Data Abort */
    phys = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!phys)
    {
        LOG(ERROR, "\tval_host_mem_alloc failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto destroy_realm;
    }

    ret = val_host_map_protected_data_unknown(&realm, phys, da_ipa, PAGE_SIZE);
    if (ret)
    {
        LOG(ERROR, "\tDATA_CREATE_UNKNOWN failed, ret = %d \n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_realm;
    }

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto destroy_realm;
    }

    /* Check that REC exit was due to Data Abort due to P1 access to IPA whose
     * HIPAS,RIPAS = UNASSIGNED,RAM */
    if (validate_rec_exit_ia(rec_exit, ia_ipa))
    {
        LOG(ERROR, "\tREC exit IA: params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
