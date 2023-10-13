/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"

#define NS_HOST_DATA 0xAAAAAAAA
#define REALM_DATA   0xBBBBBBBB

void mm_feat_s2fwb_check_1_host(void)
{
    val_host_realm_ts realm;
    val_host_rec_entry_ts *rec_entry = NULL;
    uint32_t index;
    uint64_t ret, mem_attr;
    uint64_t *addr = NULL;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

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

    mem_attr = ATTR_S2_INNER_WT_CACHEABLE | ATTR_STAGE2_MASK;
    index = val_host_map_ns_shared_region(&realm, 0x1000, mem_attr);
    if (!index)
    {
        LOG(ERROR, "\tval_host_map_ns_shared_region failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    addr = (uint64_t *)realm.granules[index].pa;
    addr[0] = NS_HOST_DATA;
    addr[1] = NS_HOST_DATA;
    addr[2] = NS_HOST_DATA;

    rec_entry = &(((val_host_rec_run_ts *)realm.run[0])->entry);
    rec_entry->gprs[1] = realm.granules[index].ipa;
    rec_entry->gprs[2] = realm.granules[index].size;
    rec_entry->flags = 0x0;
    /* Test Intent: verify FWB forces final memory attribute to Normal Cacheable
     * irrespective of the value programmed in R-EL1 stage1 tables.
     */
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

    /* Compare the REALM data */
    if (addr[0] != REALM_DATA || addr[1] != REALM_DATA || addr[2] != REALM_DATA)
    {
        LOG(ERROR, "\tREALM data mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
