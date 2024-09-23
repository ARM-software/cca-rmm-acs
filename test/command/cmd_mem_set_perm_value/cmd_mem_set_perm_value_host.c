/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_command.h"

#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512 * L3_SIZE)
#define L1_SIZE (512 * L2_SIZE)

void cmd_mem_set_perm_value_host(void)
{
    val_host_realm_ts realm;
    val_host_realm_flags1_ts realm_flags;
    uint64_t ret, ipa_base;
    val_host_rec_exit_ts *rec_exit;
    val_host_rec_enter_ts *rec_enter = NULL;
    val_smc_param_ts cmd_ret;
    val_host_rec_enter_flags_ts rec_enter_flags;

    /* Skip if RMM do not support planes */
    if (!val_host_rmm_supports_planes())
    {
        LOG(ALWAYS, "\n\tPlanes feature not supported\n", 0, 0);
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto destroy_realm;
    }

    val_memset(&realm, 0, sizeof(realm));
    val_memset(&realm_flags, 0, sizeof(realm_flags));
    val_memset(&rec_enter_flags, 0, sizeof(rec_enter_flags));

    val_host_realm_params(&realm);

    realm.num_aux_planes = 1;
    realm_flags.rtt_tree_pp = RMI_FEATURE_TRUE;

    if (val_host_rmm_supports_rtt_tree_single())
        realm_flags.rtt_tree_pp = RMI_FEATURE_FALSE;

    val_memcpy(&realm.flags1, &realm_flags, sizeof(realm.flags1));

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm, true))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    rec_exit =  &(((val_host_rec_run_ts *)realm.run[0])->exit);
    rec_enter = &(((val_host_rec_run_ts *)realm.run[0])->enter);

    if (rec_exit->exit_reason != RMI_EXIT_S2AP_CHANGE) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    ipa_base = rec_exit->s2ap_base;
    /* Create primary RTT mappings for requested IPA range in case it doesn't exist */
    while (ipa_base < rec_exit->s2ap_top)
    {
        if (create_mapping(ipa_base, false, realm.rd))
        {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto destroy_realm;
        }
        ipa_base += L2_SIZE;
    }

    /* If Realm is configured to have RTT tree per plane, create auxiliary RTT as well*/
    ipa_base = rec_exit->s2ap_base;
    if (realm_flags.rtt_tree_pp)
    {
        while (ipa_base < rec_exit->s2ap_top)
        {
            for (uint8_t i = 0; i < realm.num_aux_planes; i++)
            {
                if (val_host_create_aux_mapping(realm.rd, ipa_base, i + 1))
                {
                    val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
                    goto destroy_realm;
                }
            }
            ipa_base += L2_SIZE;
        }
    }

    ipa_base = rec_exit->s2ap_base;
    while (ipa_base < rec_exit->s2ap_top) {
        cmd_ret = val_host_rmi_rtt_set_s2ap(realm.rd, realm.rec[0], ipa_base, rec_exit->s2ap_top);
        if (cmd_ret.x0) {
            LOG(ERROR, "\nRMI_SET_S2AP failed with ret= 0x%x\n", cmd_ret.x0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
            goto destroy_realm;
        }
        ipa_base = cmd_ret.x1;
    }

    rec_enter_flags.s2ap_response = RMI_ACCEPT;
    val_memcpy(&rec_enter->flags, &rec_enter_flags, sizeof(rec_enter->flags));

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
