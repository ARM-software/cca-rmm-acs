/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "exception_common_host.h"

void exception_non_emulatable_da_host(void)
{
    val_host_realm_ts realm;
    val_host_rmifeatureregister0_ts features_0;
    val_host_rec_entry_ts *rec_entry = NULL;
    val_host_rec_exit_ts *rec_exit = NULL;
    uint32_t index;
    uint64_t ret, mem_attr;

    val_memset(&realm, 0, sizeof(realm));
    val_memset(&features_0, 0, sizeof(features_0));
    features_0.s2sz = 40;
    val_memcpy(&realm.realm_feat_0, &features_0, sizeof(features_0));

    realm.hash_algo = RMI_HASH_SHA_256;
    realm.s2_starting_level = 0;
    realm.num_s2_sl_rtts = 1;
    realm.vmid = 0;
    realm.rec_count = 1;

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm, 1))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    /* REC enter REC[0] execution */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    } else if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "\tREC_EXIT: HOST_CALL params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    mem_attr = ATTR_NORMAL_WB | ATTR_STAGE2_MASK_RO | ATTR_INNER_SHARED;
    index = val_host_map_ns_shared_region(&realm, 0x1000, mem_attr);
    if (!index)
    {
        LOG(ERROR, "\tval_host_map_ns_shared_region failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    rec_entry = &(((val_host_rec_run_ts *)realm.run[0])->entry);
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);
    rec_entry->gprs[1] = realm.granules[index].ipa;
    rec_entry->gprs[2] = realm.granules[index].size;
    rec_entry->flags = 0x0;
    /* Test Intent: UnProtected IPA, access caused a stage 2 permission fault
     * => REC exit due to Data abort.
     */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    if (!((rec_exit->exit_reason == RMI_EXIT_SYNC) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 11, 12) == ESR_ISS_SET_UER) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 9, 9) == ESR_ISS_EA) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 0, 5) == ESR_ISS_DFSC_PF_L3) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 10, 10) == ESR_FnV) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 26, 31) == ESR_EC_LOWER_EL) &&
        (VAL_EXTRACT_BITS(realm.granules[index].ipa, 8, 63) == rec_exit->hpfar)))
    {
        LOG(ERROR, "\tREC exit params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    /* On REC exit due to Non-Emulatable Data Abort, All other exit.esr fields are zero. */
    if (check_esr_mbz_feilds(rec_exit, NON_EMULATABLE_DA))
    {
        LOG(ERROR, "\tREC exit ESR params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
