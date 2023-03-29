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
#define IPA_ALIGNED_2MB 0x800000

void mm_rtt_fold_unassigned_host(void)
{
    val_host_realm_ts realm;
    val_host_rmifeatureregister0_ts features_0;
    val_host_rtt_entry_ts rtte;
    val_data_create_ts data_create;
    uint64_t ret;
    uint64_t phys;

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
    ret = val_host_init_ripas_delegate(&realm, &data_create);
    if (ret)
    {
        LOG(ERROR, "\t val_host_init_ripas_delegate failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    ret = val_host_rmi_rtt_fold(realm.rtt_l3[(realm.rtt_l3_count - 1)].rtt_addr,
                                realm.rd, data_create.ipa, VAL_RTT_MAX_LEVEL);
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

    ret = val_host_rmi_rtt_read_entry(realm.rd, data_create.ipa, VAL_RTT_MAX_LEVEL, &rtte);
    if (rtte.walk_level != (VAL_RTT_MAX_LEVEL - 1))
    {
        LOG(ERROR, "\tRTT walk level failed: %x\n", rtte.walk_level, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }

    if ((rtte.state != RMI_UNASSIGNED) || (rtte.ripas != RMI_RAM))
    {
        LOG(ERROR, "\tHIPAS-%d and RIPAS-%d state mismatch after fold\n", rtte.state, rtte.ripas);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_realm;
    }

    ret = val_host_create_rtt_levels(&realm, data_create.ipa,
                                    (uint32_t)rtte.walk_level, VAL_RTT_MAX_LEVEL,
                                    data_create.rtt_alignment);
    if (ret)
    {
        LOG(ERROR, "\tval_host_create_rtt_level failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto destroy_realm;
    }

    ret = val_host_rmi_rtt_read_entry(realm.rd, data_create.ipa, VAL_RTT_MAX_LEVEL, &rtte);
    if (rtte.walk_level != VAL_RTT_MAX_LEVEL)
    {
        LOG(ERROR, "\tRTT walk level failed: %x\n", rtte.walk_level, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_realm;
    }

    if ((rtte.state != RMI_UNASSIGNED) || (rtte.ripas != RMI_RAM))
    {
        LOG(ERROR, "\tHIPAS-%d and RIPAS-%d state mismatch after fold\n", rtte.state, rtte.ripas);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
