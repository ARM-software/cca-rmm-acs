/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "rtt_level_start.h"

static uint32_t create_realm(val_host_realm_ts *realm)
{
    uint64_t ret;
    uint64_t ipa_width;
    uint64_t pa, ipa;
    uint32_t i;
    val_host_rtt_entry_ts rtte;

    /* Populate realm with one REC */
    if (val_host_realm_setup(realm, false))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return VAL_ERROR;
    }

    ipa_width = VAL_EXTRACT_BITS(realm->s2sz, 0, 7);
    for (i = 0; i < 23; i++)
    {
        if (ipa_width != rtt_sl_start[i][0])
            continue;

        if (rtt_sl_start[i][1] & (1ull << (ipa_width - 1)))
        {
            pa = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
            ipa = rtt_sl_start[i][1];
            ret = val_host_map_unprotected(realm, pa, ipa, PAGE_SIZE, PAGE_SIZE);
            if (ret)
            {
                LOG(ERROR, "\tval_realm_map_unprotected_data failed\n", 0, 0);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
                return VAL_ERROR;
            }
            realm->granules[realm->granules_mapped_count].ipa = ipa;
            realm->granules[realm->granules_mapped_count].size = PAGE_SIZE;
            realm->granules[realm->granules_mapped_count].level = VAL_RTT_MAX_LEVEL;
            realm->granules[realm->granules_mapped_count].pa = pa;
            realm->granules_mapped_count++;
        } else {
            pa = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
            ipa = rtt_sl_start[i][1];
            if (val_host_ripas_init(realm, ipa, ipa + PAGE_SIZE, VAL_RTT_MAX_LEVEL, PAGE_SIZE))
            {
                LOG(ERROR, "\tval_host_ripas_init failed\n", 0, 0);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
                return VAL_ERROR;
            }

            ret = val_host_map_protected_data_unknown(realm, pa, ipa, PAGE_SIZE);
            if (ret)
            {
                LOG(ERROR, "\tval_host_map_protected_data_unknown failed\n", 0, 0);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
                return VAL_ERROR;
            }
        }

        ret = val_host_rmi_rtt_read_entry(realm->rd, rtt_sl_start[i][1],
                                                realm->s2_starting_level, &rtte);
        if (!RMI_STATUS(ret))
        {
            LOG(ERROR, "\tRead entry failed\n", 0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            return VAL_ERROR;
        }
    }

    /* Activate realm */
    if (val_host_realm_activate(realm))
    {
        LOG(ERROR, "\tRealm activate failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        return VAL_ERROR;
    }

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm->rec[0], realm->run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        return VAL_ERROR;
    }

    if (val_host_realm_destroy(realm->rd))
    {
        LOG(ERROR, "\tval_host_realm_destroy failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        return VAL_ERROR;
    }

    return 0;
}

void mm_rtt_level_start_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;
    uint64_t s2sz_supp = 0, i;
    uint8_t arr[5][4] = {{32, 32, 2, 4},
                         {36, 34, 2, 16},
                         {40, 40, 1, 2},
                         {42, 42, 1, 8},
                         {52, 52, 0, 16} };

    val_memset(&realm, 0, sizeof(realm));

    realm.hash_algo = RMI_HASH_SHA_256;
    realm.vmid = 0;
    realm.rec_count = 1;

    ret = val_host_rmi_features(0, &s2sz_supp);
    if (ret)
    {
        LOG(ERROR, "\tRMI Features failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_realm;
    }
    s2sz_supp = VAL_EXTRACT_BITS(s2sz_supp, 0, 7);

    LOG(DBG, "\tRMI Features s2sz supp %d %d\n", s2sz_supp, arr[0][0]);

    for (i = 0; i < 5; i++)
    {
        if (s2sz_supp < arr[i][0])
            break;
        realm.s2sz = arr[i][1];
        realm.s2_starting_level = arr[i][2];
        realm.num_s2_sl_rtts = arr[i][3];
        if (create_realm(&realm))
            goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
