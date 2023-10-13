/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_pmu.h"
#include "val_host_rmi.h"
#include "val_irq.h"

void pmu_overflow_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;
    uint64_t dfr0;
    uint64_t feature_reg;
    uint8_t pmu_num_ctrs;
    val_host_rec_exit_ts *rec_exit = NULL;
    val_host_rec_entry_ts *rec_entry = NULL;

    dfr0 = read_id_aa64dfr0_el1();
    /* Check that PMU is supported */
    if ((VAL_EXTRACT_BITS(dfr0, 8, 11) == 0x0))
    {
        LOG(ERROR, "\tPMU not supported\n", 0, 0);
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto destroy_realm;
    }

    /* Read Feature Register 0 and check for PMU support */
    val_host_rmi_features(0, &feature_reg);
    if (VAL_EXTRACT_BITS(feature_reg, 22, 22) == 0) {
        LOG(ERROR, "\tPMU not supported\n", 0, 0);
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto destroy_realm;
    }

    pmu_num_ctrs = VAL_EXTRACT_BITS(feature_reg, 23, 27);

    val_irq_enable(PMU_PPI, 0);

    val_memset(&realm, 0, sizeof(realm));
    val_host_realm_params(&realm);
    /* Enable PMU */
    realm.flags = REALM_FLAG_PMU_ENABLE;
    realm.pmu_num_ctrs = pmu_num_ctrs;

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

    if (rec_exit->exit_reason != RMI_EXIT_IRQ ||
                rec_exit->pmu_ovf_status != RMI_PMU_OVERFLOW_ACTIVE)
    {
        LOG(ERROR, "\tRec exit params mismatch, exit_reason=%x pmu_ovf_status %lx\n",
                        rec_exit->exit_reason, rec_exit->pmu_ovf_status);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    val_irq_disable(PMU_VIRQ);
    rec_entry = &(((val_host_rec_run_ts *)realm.run[0])->entry);

    /* Set No pending maintence interrupt in HCR register */
    rec_entry->gicv3_hcr = 1ULL << GICV3_HCR_EL2_NPIE;

    /* Inject PMU virtual interrupt to realm using GIC list registers */
    rec_entry->gicv3_lrs[0] = ((uint64_t)GICV3_LR_STATE_PENDING << GICV3_LR_STATE) |
                                (0ULL << GICV3_LR_HW) | (1ULL << GICV3_LR_GROUP) | PMU_VIRQ;
    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    /* Check for No pending maintence interrupt */
    if ((VAL_EXTRACT_BITS(rec_exit->gicv3_misr, GICV3_MISR_EL2_NP, GICV3_MISR_EL2_NP) != 0x1) ||
        VAL_EXTRACT_BITS(rec_exit->gicv3_hcr, GICV3_HCR_EL2_EOICOUNT, 31))
    {
        LOG(ERROR, "\tNo pending maintence interrupt check failed: %lx\n", rec_exit->gicv3_misr, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_realm:
    return;
}
