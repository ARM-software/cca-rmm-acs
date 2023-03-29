/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "mm_common_host.h"

#define ALIGNED_2MB 0x200000
#define UNPROTECTED_IPA_ALIGNED_2MB 0x8000800000

void mm_rtt_fold_u_ipa_assigned_host(void)
{
    val_host_realm_ts realm;
    val_host_rmifeatureregister0_ts features_0;
    val_host_rtt_entry_ts rtte;
    uint64_t ret;
    uint64_t phys, ipa, size;
    uint64_t i = 0;

    val_memset(&realm, 0, sizeof(realm));
    features_0.s2sz = 40;
    val_memcpy(&realm.realm_feat_0, &features_0, sizeof(features_0));

    realm.hash_algo = RMI_HASH_SHA_256;
    realm.s2_starting_level = 0;
    realm.num_s2_sl_rtts = 1;
    realm.vmid = 0;
    realm.rec_count = 1;

    /* Populate realm with one REC */
    if (val_host_realm_setup(&realm, false))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    size = 0x200000;
    phys = (uint64_t)val_host_mem_alloc(ALIGNED_2MB, size);
    if (!phys)
    {
        LOG(ERROR, "\tval_host_mem_alloc failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    ipa = UNPROTECTED_IPA_ALIGNED_2MB;
    while (i < size/PAGE_SIZE)
    {
        ret = val_host_map_unprotected(&realm,
                                       phys + i * PAGE_SIZE,
                                       ipa + i * PAGE_SIZE,
                                       PAGE_SIZE, ALIGNED_2MB);
        if (ret)
        {
            LOG(ERROR, "\t val_host_map_unprotected failed\n", 0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_realm;
        }
        i++;
    }

    ret = val_host_rmi_rtt_fold(realm.rtt_l3[(realm.rtt_l3_count - 1)].rtt_addr,
                                realm.rd, ipa, VAL_RTT_MAX_LEVEL);
    if (ret)
    {
        LOG(ERROR, "\trmi_rtt_fold failed ret = %x\n", ret, 0);
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

    /* Enter REC[0] execution */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    } else if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "\tHost call params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    ret = val_host_rmi_rtt_read_entry(realm.rd, ipa, 3, &rtte);
    if (rtte.walk_level != (VAL_RTT_MAX_LEVEL - 1))
    {
        LOG(ERROR, "\tRTT walk level failed: %x\n", rtte.walk_level, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }

    if (rtte.state != RMI_VALIDN_NS)
    {
        LOG(ERROR, "\tHIPAS-%d state mismatch after fold\n", rtte.state, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_realm;
    }

    ret = val_host_create_rtt_levels(&realm, ipa,
                                    (uint32_t)rtte.walk_level, VAL_RTT_MAX_LEVEL, ALIGNED_2MB);
    if (ret)
    {
        LOG(ERROR, "\tval_host_create_rtt_level failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto destroy_realm;
    }

    ret = val_host_rmi_rtt_read_entry(realm.rd, ipa, VAL_RTT_MAX_LEVEL, &rtte);
    if (rtte.walk_level != VAL_RTT_MAX_LEVEL)
    {
        LOG(ERROR, "\tRTT walk level failed: %x\n", rtte.walk_level, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_realm;
    }

    if (rtte.state != RMI_VALIDN_NS)
    {
        LOG(ERROR, "\tHIPAS-%d state mismatch after unfold\n", rtte.state, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto destroy_realm;
    }


    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
