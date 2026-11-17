/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "gic_common.h"
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
    bool pmu_irq_enabled = false;
    bool gic_programmed = false;

    dfr0 = read_id_aa64dfr0_el1();
    /* Check that PMU is supported */
    if ((VAL_EXTRACT_BITS(dfr0, 8, 11) == 0x0))
    {
        LOG(ERROR, "PMU not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto destroy_realm;
    }

    /* Read Feature Register 0 and check for PMU support */
    val_host_rmi_features(0, &feature_reg);
    if (VAL_EXTRACT_BITS(feature_reg, 26, 26) == 0) {
        LOG(ERROR, "PMU not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto destroy_realm;
    }

    pmu_num_ctrs = VAL_EXTRACT_BITS(feature_reg, 27, 31);
    if (pmu_num_ctrs == 0) {
        LOG(TEST, "No Realm-assigned PMU counter is available\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto destroy_realm;
    }

    val_irq_enable(PMU_PPI, 0);
    pmu_irq_enabled = true;

    val_memset(&realm, 0, sizeof(realm));
    val_host_realm_params(&realm);
    /* Enable PMU */
    realm.flags = REALM_FLAG_PMU_ENABLE;
    realm.pmu_num_ctrs = pmu_num_ctrs;

    /* Populate realm with one REC */
    if (val_host_realm_setup(&realm, true))
    {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    if (rec_exit->exit_reason != RMI_EXIT_IRQ ||
                rec_exit->pmu_ovf_status != RMI_PMU_OVERFLOW_ACTIVE)
    {
        LOG(ERROR, "Rec exit params mismatch, exit_reason=%x pmu_ovf_status %lx\n",
                        rec_exit->exit_reason, rec_exit->pmu_ovf_status);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    /* Keep the physical PMU PPI from interrupting the next REC entry. */
    val_irq_disable(PMU_PPI);
    pmu_irq_enabled = false;

    /* Inject the virtual PMU interrupt through the Host GIC list register. */
    gic_host_ich_program(1ULL << GICV3_HCR_EL2_EN,
                         (1ULL << 1) | (0xffULL << 24),
                         gic_lr(PMU_VIRQ, GICV3_LR_STATE_PENDING));
    gic_programmed = true;

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "Expected Realm Host-call exit, reason=%lx "
            "pmu_ovf_status=%lx pending_intid=%lx\n",
            rec_exit->exit_reason, rec_exit->pmu_ovf_status,
            read_icc_hppir1_el1());
        LOG(ERROR, "Host GIC state: ICH_MISR_EL2=%lx ICH_HCR_EL2=%lx "
            "ICH_LR0_EL2=%lx\n", read_ich_misr_el2(),
            read_ich_hcr_el2(), read_ich_lr0_el2());
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    if (!IS_TEST_START(val_get_status()))
        goto destroy_realm;

    /* The Realm cleared the overflow before this Host-call exit. */
    if (rec_exit->pmu_ovf_status != RMI_PMU_OVERFLOW_NOT_ACTIVE)
    {
        LOG(ERROR, "Expected inactive PMU overflow, status=%lx\n",
                        rec_exit->pmu_ovf_status);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_realm:
    if (gic_programmed)
        gic_host_ich_clear();

    if (pmu_irq_enabled)
        val_irq_disable(PMU_PPI);

    return;
}
