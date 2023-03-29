 /*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "exception_common_host.h"

void exception_rec_exit_hostcall_host(void)
{
    val_host_realm_ts realm = {0,};
    uint64_t ret = 0;
    val_host_rec_entry_ts *rec_entry = NULL;
    val_host_rec_exit_ts *rec_exit = NULL;
    uint64_t ec = 0;
    uint64_t imm = 0;
    val_host_rec_entry_flags_ts rec_entry_flags;
    val_host_rmifeatureregister0_ts featureregister0 = {0,};

    val_memset(&realm, 0, sizeof(realm));
    val_memset(&rec_entry_flags, 0, sizeof(rec_entry_flags));

    featureregister0.s2sz = 40;
    featureregister0.hash_sha_256 = 1;
    val_memcpy(&realm.realm_feat_0, &featureregister0, sizeof(val_host_rmifeatureregister0_ts));
    realm.hash_algo = RMI_SHA256;
    realm.s2_starting_level = 0;
    realm.num_s2_sl_rtts = 1;
    realm.vmid = 0;
    realm.rec_count = 1;

    /* Populate realm with one RECs*/
    if (val_host_realm_setup(&realm, 1))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    rec_entry = &(((val_host_rec_run_ts *)realm.run[0])->entry);
    rec_exit =  &(((val_host_rec_run_ts *)realm.run[0])->exit);

   /* REC enter Before the GPR corruption */
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

    LOG(ALWAYS, "\tRec enter HostCall verified\n", 0, 0);
    /* populate the rec exit details into the rec enter and corrupt some possible gprs
     * in the range 0-6
     */
    exception_copy_exit_to_entry(rec_entry, rec_exit);
    rec_entry->gprs[6] = 0;

    rec_entry_flags.trap_wfe = 0;
    rec_entry_flags.trap_wfi = 0;

    val_memcpy(&rec_entry->flags, &rec_entry_flags, sizeof(rec_entry_flags));

    /* REC enter after the GPR corruption */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret != 0)
    {
        LOG(ERROR, "\tRec Exit not dueto  Hostcall(testcase end check), ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
    }
destroy_realm:
    return;
}
