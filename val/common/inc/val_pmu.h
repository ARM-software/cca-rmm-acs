/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_PMU_H_
#define _VAL_PMU_H_

#include "pal_interfaces.h"
#include "val_def.h"
#include "val_framework.h"

#define MAX_COUNTERS        31

/* Clear bits P0-P30, C and F0 */
#define PMU_CLEAR_ALL   0xFFFFFFFFF

/* PMCCFILTR_EL0 mask */
#define PMCCFILTR_EL0_MASK (      \
    PMCCFILTR_EL0_P_BIT | \
    PMCCFILTR_EL0_U_BIT | \
    PMCCFILTR_EL0_NSK_BIT   | \
    PMCCFILTR_EL0_NSH_BIT   | \
    PMCCFILTR_EL0_M_BIT | \
    PMCCFILTR_EL0_RLK_BIT   | \
    PMCCFILTR_EL0_RLU_BIT   | \
    PMCCFILTR_EL0_RLH_BIT)

/* PMEVTYPER<n>_EL0 mask */
#define PMEVTYPER_EL0_MASK (      \
    PMEVTYPER_EL0_P_BIT | \
    PMEVTYPER_EL0_U_BIT | \
    PMEVTYPER_EL0_NSK_BIT   | \
    PMEVTYPER_EL0_NSU_BIT   | \
    PMEVTYPER_EL0_NSH_BIT   | \
    PMEVTYPER_EL0_M_BIT | \
    PMEVTYPER_EL0_RLK_BIT   | \
    PMEVTYPER_EL0_RLU_BIT   | \
    PMEVTYPER_EL0_RLH_BIT   | \
    PMEVTYPER_EL0_EVTCOUNT_BITS)

/* PMSELR_EL0 mask */
#define PMSELR_EL0_MASK     0x1F
#define RANDOM_NUMBER		0xA1B2C3D4

#define WRITE_PMEV_REGS(n) {                    \
    pmu_ptr->pmevcntr_el0[n] = RANDOM_NUMBER;            \
    write_pmevcntrn_el0(n, pmu_ptr->pmevcntr_el0[n]);   \
    pmu_ptr->pmevtyper_el0[n] = RANDOM_NUMBER & PMEVTYPER_EL0_MASK;\
    write_pmevtypern_el0(n, pmu_ptr->pmevtyper_el0[n]); \
}

#define CHECK_PMEV_REG(n, reg) {                \
    read_val = read_##reg##n_el0(n);            \
    if (read_val != pmu_ptr->reg##_el0[n]) {        \
        return false;                   \
    }                           \
}

#define CHECK_PMEV_REGS(n) {        \
    CHECK_PMEV_REG(n, pmevcntr);    \
    CHECK_PMEV_REG(n, pmevtyper);   \
}

#define WRITE_PMREG(reg, mask) {        \
    pmu_ptr->reg = RANDOM_NUMBER & mask; \
    write_##reg(pmu_ptr->reg);      \
}

#define CHECK_PMREG(reg) {                  \
    read_val = read_##reg();                \
    val = pmu_ptr->reg;                 \
    if (read_val != val) {                  \
        return false;                   \
    }                           \
}

/* PMUv3 events */
#define PMU_EVT_SW_INCR     0x0
#define PMU_EVT_INST_RETIRED    0x8
#define PMU_EVT_CPU_CYCLES  0x11
#define PMU_EVT_MEM_ACCESS  0x13

#define PRE_OVERFLOW        ~(0xF)

struct pmu_registers {
    unsigned long pmcr_el0;
    unsigned long pmcntenset_el0;
    unsigned long pmovsset_el0;
    unsigned long pmintenset_el1;
    unsigned long pmccntr_el0;
    unsigned long pmccfiltr_el0;
    unsigned long pmuserenr_el0;

    unsigned long pmevcntr_el0[MAX_COUNTERS];
    unsigned long pmevtyper_el0[MAX_COUNTERS];

    unsigned long pmselr_el0;
    unsigned long pmxevcntr_el0;
    unsigned long pmxevtyper_el0;

} __aligned(CACHE_WRITEBACK_GRANULE);

void enable_counting(void);
void disable_counting(void);
void enable_event_counter(uint32_t ctr_num);
void pmu_reset(void);

#endif /* _VAL_PMU_H_ */
