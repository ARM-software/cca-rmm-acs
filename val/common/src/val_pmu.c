/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_pmu.h"
#include "val_irq.h"
#include "pal_interfaces.h"
#include "pal_gic_common.h"
#include "val.h"

void enable_counting(void)
{
    write_pmcr_el0(read_pmcr_el0() | PMCR_EL0_E_BIT);
    /* This function means we are about to use the PMU, synchronize */
    isb();
}

void disable_counting(void)
{
    write_pmcr_el0(read_pmcr_el0() & ~PMCR_EL0_E_BIT);
    /* We also rely that disabling really did work */
    isb();
}

void enable_event_counter(uint32_t ctr_num)
{
    /*
     * Set PMEVTYPER_EL0.U != PMEVTYPER_EL0.RLU
     * to disable event counting in Realm EL0.
     * Set PMEVTYPER_EL0.P = PMEVTYPER_EL0.RLK
     * to enable counting in Realm EL1.
     * Set PMEVTYPER_EL0.NSH = PMEVTYPER_EL0.RLH
     * to disable event counting in Realm EL2.
     */
    write_pmevtypern_el0(ctr_num,
            PMEVTYPER_EL0_U_BIT |
            PMEVTYPER_EL0_P_BIT | PMEVTYPER_EL0_RLK_BIT |
            PMEVTYPER_EL0_NSH_BIT | PMEVTYPER_EL0_RLH_BIT |
            (PMU_EVT_INST_RETIRED & PMEVTYPER_EL0_EVTCOUNT_BITS));
    write_pmcntenset_el0(read_pmcntenset_el0() |
        PMCNTENSET_EL0_P_BIT(ctr_num));
    isb();
}

void pmu_reset(void)
{
    /* Reset all counters */
    write_pmcr_el0(read_pmcr_el0() |
            PMCR_EL0_DP_BIT | PMCR_EL0_C_BIT | PMCR_EL0_P_BIT);

    /* Disable all counters */
    write_pmcntenclr_el0(PMU_CLEAR_ALL);

    /* Clear overflow status */
    write_pmovsclr_el0(PMU_CLEAR_ALL);

    /* Disable overflow interrupts on all counters */
    write_pmintenclr_el1(PMU_CLEAR_ALL);
    isb();
}
