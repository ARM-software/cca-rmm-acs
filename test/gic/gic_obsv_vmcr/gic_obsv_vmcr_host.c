/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"

void gic_obsv_vmcr_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;
    val_host_rec_exit_ts *rec_exit = NULL;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    /* Populate realm with one REC */
    if (val_host_realm_setup(&realm, true))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    /* Read and compare the VMCR priority field */
    if ((VAL_EXTRACT_BITS(rec_exit->gicv3_vmcr, 24, 31) !=
                        VAL_EXTRACT_BITS(rec_exit->gprs[1], 0, 7)))
    {
        LOG(ERROR, "\tGIC VMCR priority mismatch, received %x expected %x\n",
                        VAL_EXTRACT_BITS(rec_exit->gicv3_vmcr, 24, 31),
                        VAL_EXTRACT_BITS(rec_exit->gprs[1], 0, 7));
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    /* Read and compare the VMCR EOIM field */
    if ((VAL_EXTRACT_BITS(rec_exit->gicv3_vmcr, 9, 9) !=
                        VAL_EXTRACT_BITS(rec_exit->gprs[2], 1, 1)))
    {
        LOG(ERROR, "\tGIC VMCR EOIM mismatch, received %x expected %x\n",
                        VAL_EXTRACT_BITS(rec_exit->gicv3_vmcr, 9, 9),
                        VAL_EXTRACT_BITS(rec_exit->gprs[2], 1, 1));
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    /* Read and compare the VMCR IGRPEN1 field */
    if ((VAL_EXTRACT_BITS(rec_exit->gicv3_vmcr, 1, 1) !=
                        VAL_EXTRACT_BITS(rec_exit->gprs[3], 0, 0)))
    {
        LOG(ERROR, "\tGIC VMCR IGRPEN1 mismatch, received %x expected %x\n",
                        VAL_EXTRACT_BITS(rec_exit->gicv3_vmcr, 1, 1),
                        VAL_EXTRACT_BITS(rec_exit->gprs[3], 0, 0));
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    /* Read and compare the VMCR BPR0 field */
    if ((VAL_EXTRACT_BITS(rec_exit->gicv3_vmcr, 21, 23) !=
                        VAL_EXTRACT_BITS(rec_exit->gprs[4], 0, 2)))
    {
        LOG(ERROR, "\tGIC VMCR BPR0 mismatch, received %x expected %x\n",
                        VAL_EXTRACT_BITS(rec_exit->gicv3_vmcr, 21, 23),
                        VAL_EXTRACT_BITS(rec_exit->gprs[4], 0, 2));
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
