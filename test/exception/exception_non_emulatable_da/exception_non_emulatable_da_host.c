/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_helpers.h"

void exception_non_emulatable_da_host(void)
{
    val_host_realm_ts realm;
    val_host_rec_enter_ts *rec_enter = NULL;
    val_host_rec_exit_ts *rec_exit = NULL;
    uint32_t index;
    uint64_t ret, mem_attr;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm, 1))
    {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    /* REC enter REC[0] execution */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    } else if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "REC_EXIT: HOST_CALL params mismatch\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    mem_attr = ATTR_NORMAL_WB | ATTR_STAGE2_MASK_WO;
    index = val_host_map_ns_shared_region(&realm, 0x1000, mem_attr);
    if (!index)
    {
        LOG(ERROR, "val_host_map_ns_shared_region failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    rec_enter = &(((val_host_rec_run_ts *)realm.run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);
    rec_enter->gprs[1] = realm.granules[index].ipa;
    rec_enter->gprs[2] = realm.granules[index].size;
    rec_enter->flags = 0x0;
    /* Test Intent: UnProtected IPA, access caused a stage 2 permission fault
     * => REC exit due to Data abort.
     */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    /* validates the Rec Exit due to DA */
    if (validate_rec_exit_da(rec_exit, realm.granules[index].ipa,
                                ESR_ISS_DFSC_PF_L3, NON_EMULATABLE_DA, 0))
    {
        LOG(ERROR, "REC exit ESR params mismatch\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
