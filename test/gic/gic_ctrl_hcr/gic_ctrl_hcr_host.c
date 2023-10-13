/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_irq.h"

void gic_ctrl_hcr_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;
    val_host_rec_entry_ts *rec_entry = NULL;
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

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    rec_entry = &(((val_host_rec_run_ts *)realm.run[0])->entry);
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);

    /* Set No pending maintence interrupt in HCR register */
    rec_entry->gicv3_hcr = 1ULL << GICV3_HCR_EL2_NPIE;

    /* Inject interrupt to realm using GIC list registers */
    rec_entry->gicv3_lrs[0] = ((uint64_t)GICV3_LR_STATE_PENDING << GICV3_LR_STATE) |
                                (0ULL << GICV3_LR_HW) | (1ULL << GICV3_LR_GROUP) | SPI_vINTID;
    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    /* Check for No pending maintence interrupt */
    if ((VAL_EXTRACT_BITS(rec_exit->gicv3_misr, GICV3_MISR_EL2_NP, GICV3_MISR_EL2_NP) != 0x1) ||
        VAL_EXTRACT_BITS(rec_exit->gicv3_hcr, GICV3_HCR_EL2_EOICOUNT, 31))
    {
        LOG(ERROR, "\tNo pending maintence interrupt check failed: %lx\n", rec_exit->gicv3_misr, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    /* Reset the GIC list register */
    rec_entry->gicv3_lrs[0] = 0x0;
    /* Reset the GIC HCR register */
    rec_entry->gicv3_hcr = 0x0;

    /* Set Underflow maintence interrupt in HCR register */
    rec_entry->gicv3_hcr = 1ULL << GICV3_HCR_EL2_UIE;

    /* Inject interrupt to realm using GIC list registers */
    rec_entry->gicv3_lrs[0] = ((uint64_t)GICV3_LR_STATE_PENDING << GICV3_LR_STATE) |
                            (0ULL << GICV3_LR_HW) | (1ULL << GICV3_LR_GROUP) | SPI_vINTID;
    rec_entry->gicv3_lrs[1] = ((uint64_t)GICV3_LR_STATE_PENDING << GICV3_LR_STATE) |
                            (0ULL << GICV3_LR_HW) | (1ULL << GICV3_LR_GROUP) | PPI_vINTID;
    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    /* Check for Underflow maintence interrupt */
    if ((VAL_EXTRACT_BITS(rec_exit->gicv3_misr, GICV3_MISR_EL2_U, GICV3_MISR_EL2_U) != 0x1) ||
        VAL_EXTRACT_BITS(rec_exit->gicv3_hcr, GICV3_HCR_EL2_EOICOUNT, 31))
    {
        LOG(ERROR, "\tUnderflow maintence interrupt check failed: %lx\n", rec_exit->gicv3_misr, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    /* Reset the GIC list register */
    rec_entry->gicv3_lrs[0] = 0x0;
    rec_entry->gicv3_lrs[1] = 0x0;
    /* Reset the GIC HCR register */
    rec_entry->gicv3_hcr = 0x0;

    /* Set Not Present maintence interrupt in HCR register */
    rec_entry->gicv3_hcr = 1ULL << GICV3_HCR_EL2_LRENPIE;

    /* Inject interrupt to realm using GIC list registers */
    rec_entry->gicv3_lrs[0] = ((uint64_t)GICV3_LR_STATE_PENDING << GICV3_LR_STATE) |
                                (0ULL << GICV3_LR_HW) | (1ULL << GICV3_LR_GROUP) | SGI_vINTID;
    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    /* Check for Not Present maintence interrupt */
    if ((VAL_EXTRACT_BITS(rec_exit->gicv3_misr, GICV3_MISR_EL2_LRENP, GICV3_MISR_EL2_LRENP) != 0x1)
        || VAL_EXTRACT_BITS(rec_exit->gicv3_hcr, GICV3_HCR_EL2_EOICOUNT, 31) != 0x1)
    {
        LOG(ERROR, "\tNot Present maintence interrupt failed: %lx\n", rec_exit->gicv3_misr, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
