/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"

#define ALIGNED_2MB 0x200000
#define IPA_ALIGNED_2MB 0x800000

void mm_rtt_fold_assigned_host(void)
{
    val_host_realm_ts realm;
    val_host_rtt_entry_ts rtte;
    val_host_rtt_entry_ts b_fold_rtt, a_fold_rtt;
    val_host_rtt_entry_ts unfold_rtt;
    val_data_create_ts data_create;
    uint64_t ret, rtt = 0;
    uint64_t phys, p_rtte_addr;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    /* Populate realm with one REC */
    if (val_host_realm_setup(&realm, false))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    data_create.size = 0x200000;
    phys = (uint64_t)val_host_mem_alloc(ALIGNED_2MB, (2 * data_create.size));
    if (!phys)
    {
        LOG(ERROR, "\tval_host_mem_alloc failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    data_create.src_pa = phys;
    data_create.target_pa = phys + data_create.size;
    data_create.ipa = IPA_ALIGNED_2MB;
    data_create.rtt_alignment = ALIGNED_2MB;
    ret = val_host_map_protected_data_to_realm(&realm, &data_create);
    if (ret)
    {
        LOG(ERROR, "\tval_host_map_protected_data_to_realm failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    /* Save the parent rtte.addr for comparision after fold */
    ret = val_host_rmi_rtt_read_entry(realm.rd, data_create.ipa, VAL_RTT_MAX_LEVEL - 1, &rtte);
    if (ret)
    {
        LOG(ERROR, "\trtt_read_entry failed ret = %x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    p_rtte_addr = OA(rtte.desc);

    /* Save the fold.addr and fold.ripas for comparision after fold */
    ret = val_host_rmi_rtt_read_entry(realm.rd, data_create.ipa, VAL_RTT_MAX_LEVEL, &b_fold_rtt);
    if (ret)
    {
        LOG(ERROR, "\trtt_read_entry failed ret = %x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    ret = val_host_rmi_rtt_fold(realm.rd, data_create.ipa, VAL_RTT_MAX_LEVEL, &rtt);
    if (ret)
    {
        LOG(ERROR, "\trmi_rtt_fold failed ret = %x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    if (rtt != p_rtte_addr)
    {
        LOG(ERROR, "\tFold rtt addr mismatch, expected %lx received %lx\n",
                                    p_rtte_addr, rtt);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    /* Activate realm */
    if (val_host_realm_activate(&realm))
    {
        LOG(ERROR, "\tRealm activate failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }

    /* Enter REC[0] execution */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_realm;
    } else if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "\tHost call params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto destroy_realm;
    }

    /* Compare the state and attributes after the FOLD */
    ret = val_host_rmi_rtt_read_entry(realm.rd, data_create.ipa,
                                VAL_RTT_MAX_LEVEL - 1, &a_fold_rtt);
    if (ret)
    {
        LOG(ERROR, "\trtt_read_entry failed ret = %x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_realm;
    }

    if (b_fold_rtt.state != a_fold_rtt.state ||
        b_fold_rtt.desc != a_fold_rtt.desc ||
        b_fold_rtt.ripas != a_fold_rtt.ripas ||
        b_fold_rtt.walk_level != (a_fold_rtt.walk_level + 1))
    {
        LOG(ERROR, "\tFOLD Rtt params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto destroy_realm;
    }


    ret = val_host_rmi_rtt_read_entry(realm.rd, (data_create.ipa + 0x80000),
                                                VAL_RTT_MAX_LEVEL, &a_fold_rtt);
    if (ret)
    {
        LOG(ERROR, "\trtt_read_entry failed ret = %x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto destroy_realm;
    }

    if (b_fold_rtt.state != a_fold_rtt.state ||
        b_fold_rtt.ripas != a_fold_rtt.ripas ||
        b_fold_rtt.desc != a_fold_rtt.desc ||
        b_fold_rtt.walk_level != (a_fold_rtt.walk_level + 1))
    {
        LOG(ERROR, "\tFOLD Rtt params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(14)));
        goto destroy_realm;
    }

    ret = val_host_create_rtt_levels(&realm, data_create.ipa,
                                    (uint32_t)a_fold_rtt.walk_level, VAL_RTT_MAX_LEVEL,
                                    data_create.rtt_alignment);
    if (ret)
    {
        LOG(ERROR, "\tval_host_create_rtt_level failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(15)));
        goto destroy_realm;
    }

    ret = val_host_rmi_rtt_read_entry(realm.rd, data_create.ipa, VAL_RTT_MAX_LEVEL, &unfold_rtt);
    if (ret)
    {
        LOG(ERROR, "\trtt_read_entry failed ret = %x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(16)));
        goto destroy_realm;
    }

    if (val_memcmp(&b_fold_rtt, &unfold_rtt, sizeof(b_fold_rtt)))
    {
        LOG(ERROR, "\tUNFOLD Rtt params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(17)));
        goto destroy_realm;
    }

    ret = val_host_rmi_rtt_read_entry(realm.rd, (data_create.ipa + 0x80000),
                                                VAL_RTT_MAX_LEVEL, &unfold_rtt);
    if (ret)
    {
        LOG(ERROR, "\trtt_read_entry failed ret = %x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(18)));
        goto destroy_realm;
    }

    if (b_fold_rtt.state != unfold_rtt.state ||
        b_fold_rtt.ripas != unfold_rtt.ripas ||
        (b_fold_rtt.desc + 0x80000) != unfold_rtt.desc ||
        b_fold_rtt.walk_level != unfold_rtt.walk_level)
    {
        LOG(ERROR, "\tFOLD Rtt params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(19)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
