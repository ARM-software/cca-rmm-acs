/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_irq.h"

#define vINTID 0x35
#define pINTID 0x32

void gic_ctrl_list_invalid_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;
    val_host_rec_entry_ts *rec_entry = NULL;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    /* Populate realm with one REC */
    if (val_host_realm_setup(&realm, true))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    rec_entry = &(((val_host_rec_run_ts *)realm.run[0])->entry);

    /* Set State[63:62]= 0x1(pending) and HW[61]=0x1 fields */
    rec_entry->gicv3_lrs[0] = (1ULL << GICV3_LR_STATE) | (1ULL << GICV3_LR_HW) |
                             ((uint64_t)pINTID << GICV3_LR_pINTID) | vINTID;
    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (!ret)
    {
        LOG(ERROR, "\tgicv3_lr invalid fields validation failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    /* Set State[63:62]= 0x2(active) and HW[61]=0x1 fields */
    rec_entry->gicv3_lrs[0] = (2ULL << GICV3_LR_STATE) | (1ULL << GICV3_LR_HW) |
                             ((uint64_t)pINTID << GICV3_LR_pINTID) | vINTID;
    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (!ret)
    {
        LOG(ERROR, "\tgicv3_lr invalid fields validation failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    /* Set State[63:62]= 0x3(pending and active) and HW[61]=0x1 fields */
    rec_entry->gicv3_lrs[0] = (3ULL << GICV3_LR_STATE) | (1ULL << GICV3_LR_HW) |
                             ((uint64_t)pINTID << GICV3_LR_pINTID) | vINTID;
    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (!ret)
    {
        LOG(ERROR, "\tgicv3_lr invalid fields validation failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
