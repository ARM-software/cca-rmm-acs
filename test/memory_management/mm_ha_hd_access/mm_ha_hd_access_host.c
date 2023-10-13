/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_realm.h"
#include "val_host_helpers.h"

#define PROTECTED_IPA 0x800000

void mm_ha_hd_access_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;
    uint64_t index, mem_attr;
    val_host_rec_entry_ts *rec_entry = NULL;
    uint64_t ripas_ipa, ripas_size = 0x1000;
    uint64_t phys;
    val_data_create_ts data_create;

    val_memset(&realm, 0, sizeof(realm));
    val_host_realm_params(&realm);

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm, false))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    data_create.size = ripas_size;
    phys = (uint64_t)val_host_mem_alloc(PAGE_SIZE, (2 * data_create.size));
    if (!phys)
    {
        LOG(ERROR, "\tval_host_mem_alloc failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    data_create.src_pa = phys;
    data_create.target_pa = phys + data_create.size;
    data_create.ipa = PROTECTED_IPA;
    data_create.rtt_alignment = PAGE_SIZE;
    ret = val_host_map_protected_data_to_realm(&realm, &data_create);
    if (ret)
    {
        LOG(ERROR, "\tval_host_map_protected_data_to_realm failed\n", 0, 0);
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

    rec_entry = &(((val_host_rec_run_ts *)realm.run[0])->entry);
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
    ripas_ipa = PROTECTED_IPA;
    rec_entry->gprs[1] = ripas_ipa;
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    mem_attr = ATTR_NORMAL_WB | ATTR_STAGE2_MASK_RO | ATTR_INNER_SHARED | ATTR_DBM;
    index = val_host_map_ns_shared_region(&realm, 0x1000, mem_attr);
    if (index)
    {
        LOG(ERROR, "\trmi_rtt_map_unprotected must return error for DBM attribute\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
