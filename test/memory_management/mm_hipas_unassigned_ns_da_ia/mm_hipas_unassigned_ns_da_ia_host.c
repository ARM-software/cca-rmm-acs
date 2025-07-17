/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"


void mm_hipas_unassigned_ns_da_ia_host(void)
{
    val_host_realm_ts realm;
    val_host_rec_enter_ts *rec_enter = NULL;
    val_host_rec_exit_ts *rec_exit = NULL;
    uint32_t index;
    uint64_t ret, mem_attr;
    uint64_t top;

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

    mem_attr = ATTR_NORMAL_WB | ATTR_STAGE2_MASK;
    index = val_host_map_ns_shared_region(&realm, 0x1000, mem_attr);
    if (!index)
    {
        LOG(ERROR, "val_host_map_ns_shared_region failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    ret = val_host_rmi_rtt_unmap_unprotected(realm.rd, realm.granules[index].ipa,
                                            realm.granules[index].level, &top);
    if (ret)
    {
        LOG(ERROR, "val_rmi_rtt_unmap_unprotected failed, ipa=0x%x, ret=0x%x\n",
                                                             realm.granules[index].ipa, ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    rec_enter = &(((val_host_rec_run_ts *)realm.run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);
    rec_enter->gprs[1] = realm.granules[index].ipa;
    rec_enter->gprs[2] = realm.granules[index].size;
    rec_enter->flags = 0x0;
    /* Test Intent: UnProtected IPA, HIPAS=UNASSIGNED_NS data access
     * => REC exit due to Data abort
     */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    if (!((rec_exit->exit_reason == RMI_EXIT_SYNC) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 11, 12) == ESR_ISS_SET_UER) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 9, 9) == ESR_ISS_EA) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 0, 5) == ESR_ISS_DFSC_TTF_L3) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 26, 31) == ESR_EC_LOWER_EL) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 24, 24) == ESR_ISV_VALID) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 22, 23) == ESR_SAS_WORD) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 6, 6) == ESR_WnR_WRITE) &&
        (VAL_EXTRACT_BITS(rec_exit->esr, 10, 10) == ESR_FnV) &&
        (VAL_EXTRACT_BITS(realm.granules[index].ipa, 8, 63) == rec_exit->hpfar)))
    {
        LOG(ERROR, "REC exit params mismatch\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    /* Inject SEA to Realm */
    rec_enter->flags = 0x2;
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }
    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
