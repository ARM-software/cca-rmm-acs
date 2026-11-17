/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"
#include "pal_arch_helpers.h"
#include "val_host_rmi.h"
#include "val_host_realm.h"
#include "val_irq.h"
#include "val_timer.h"
#include "val_arch.h"
#include "val_framework.h"


#define GIC_LR_PRIORITY_SHIFT    48
#define GIC_LOW_PRIORITY         0x80U
#define GIC_ICH_VTR_PREBITS_SHIFT 26U
#define GIC_ICH_VTR_PREBITS_MASK  0x7U
#define GIC_ICH_MAX_APR_NUM_REGS  4U

static inline uint64_t gic_read_ich_hcr_el2(void)
{
    return read_ich_hcr_el2();
}

static inline uint64_t gic_read_ich_vtr_el2(void)
{
    return read_ich_vtr_el2();
}

static inline void gic_write_ich_hcr_el2(uint64_t val)
{
    write_ich_hcr_el2(val);
}

static inline uint64_t gic_read_ich_vmcr_el2(void)
{
    return read_ich_vmcr_el2();
}

static inline void gic_write_ich_vmcr_el2(uint64_t val)
{
    write_ich_vmcr_el2(val);
}

static inline uint64_t gic_read_ich_ap0r_el2(uint32_t index)
{
    uint64_t val = 0U;

    switch (index) {
    case 0U:
        val = read_ich_ap0r0_el2();
        break;
    case 1U:
        val = read_ich_ap0r1_el2();
        break;
    case 2U:
        val = read_ich_ap0r2_el2();
        break;
    case 3U:
        val = read_ich_ap0r3_el2();
        break;
    default:
        break;
    }

    return val;
}

static inline void gic_write_ich_ap0r_el2(uint32_t index, uint64_t val)
{
    switch (index) {
    case 0U:
        write_ich_ap0r0_el2(val);
        break;
    case 1U:
        write_ich_ap0r1_el2(val);
        break;
    case 2U:
        write_ich_ap0r2_el2(val);
        break;
    case 3U:
        write_ich_ap0r3_el2(val);
        break;
    default:
        break;
    }
}

static inline uint64_t gic_read_ich_ap1r_el2(uint32_t index)
{
    uint64_t val = 0U;

    switch (index) {
    case 0U:
        val = read_ich_ap1r0_el2();
        break;
    case 1U:
        val = read_ich_ap1r1_el2();
        break;
    case 2U:
        val = read_ich_ap1r2_el2();
        break;
    case 3U:
        val = read_ich_ap1r3_el2();
        break;
    default:
        break;
    }

    return val;
}

static inline void gic_write_ich_ap1r_el2(uint32_t index, uint64_t val)
{
    switch (index) {
    case 0U:
        write_ich_ap1r0_el2(val);
        break;
    case 1U:
        write_ich_ap1r1_el2(val);
        break;
    case 2U:
        write_ich_ap1r2_el2(val);
        break;
    case 3U:
        write_ich_ap1r3_el2(val);
        break;
    default:
        break;
    }
}

static inline uint64_t gic_read_ich_lr_el2(uint32_t index)
{
    uint64_t val = 0U;

    switch (index) {
    case 0U:
        val = read_ich_lr0_el2();
        break;
    case 1U:
        val = read_ich_lr1_el2();
        break;
    case 2U:
        val = read_ich_lr2_el2();
        break;
    case 3U:
        val = read_ich_lr3_el2();
        break;
    case 4U:
        val = read_ich_lr4_el2();
        break;
    case 5U:
        val = read_ich_lr5_el2();
        break;
    case 6U:
        val = read_ich_lr6_el2();
        break;
    case 7U:
        val = read_ich_lr7_el2();
        break;
    case 8U:
        val = read_ich_lr8_el2();
        break;
    case 9U:
        val = read_ich_lr9_el2();
        break;
    case 10U:
        val = read_ich_lr10_el2();
        break;
    case 11U:
        val = read_ich_lr11_el2();
        break;
    case 12U:
        val = read_ich_lr12_el2();
        break;
    case 13U:
        val = read_ich_lr13_el2();
        break;
    case 14U:
        val = read_ich_lr14_el2();
        break;
    case 15U:
        val = read_ich_lr15_el2();
        break;
    default:
        break;
    }

    return val;
}

static inline void gic_write_ich_lr_el2(uint32_t index, uint64_t val)
{
    switch (index) {
    case 0U:
        write_ich_lr0_el2(val);
        break;
    case 1U:
        write_ich_lr1_el2(val);
        break;
    case 2U:
        write_ich_lr2_el2(val);
        break;
    case 3U:
        write_ich_lr3_el2(val);
        break;
    case 4U:
        write_ich_lr4_el2(val);
        break;
    case 5U:
        write_ich_lr5_el2(val);
        break;
    case 6U:
        write_ich_lr6_el2(val);
        break;
    case 7U:
        write_ich_lr7_el2(val);
        break;
    case 8U:
        write_ich_lr8_el2(val);
        break;
    case 9U:
        write_ich_lr9_el2(val);
        break;
    case 10U:
        write_ich_lr10_el2(val);
        break;
    case 11U:
        write_ich_lr11_el2(val);
        break;
    case 12U:
        write_ich_lr12_el2(val);
        break;
    case 13U:
        write_ich_lr13_el2(val);
        break;
    case 14U:
        write_ich_lr14_el2(val);
        break;
    case 15U:
        write_ich_lr15_el2(val);
        break;
    default:
        break;
    }
}

uint64_t gic_lr_prio(uint32_t intid, uint32_t state, uint32_t prio)
{
    return ((uint64_t)state << GICV3_LR_STATE) |
           (0ULL << GICV3_LR_HW) |
           (1ULL << GICV3_LR_GROUP) |
           ((uint64_t)(prio & 0xffU) << GIC_LR_PRIORITY_SHIFT) |
           intid;
}

uint64_t gic_lr(uint32_t intid, uint32_t state)
{
    return gic_lr_prio(intid, state, GIC_LOW_PRIORITY);
}

void gic_host_ich_clear(void)
{
    uint32_t i;
    uint32_t num_aprs;

    num_aprs = gic_host_ich_num_aprs();

    gic_write_ich_hcr_el2(0U);
    gic_write_ich_vmcr_el2(0U);
    for (i = 0U; i < num_aprs; i++) {
        gic_write_ich_ap0r_el2(i, 0U);
        gic_write_ich_ap1r_el2(i, 0U);
    }
    for (i = 0U; i < VAL_REC_GIC_NUM_LRS; i++) {
        gic_write_ich_lr_el2(i, 0U);
    }
    isb();
}

static void gic_host_ich_program_with_ap1r0(uint64_t hcr, uint64_t vmcr, uint64_t lr0,
                                       uint64_t ap1r0)
{
    gic_write_ich_vmcr_el2(vmcr);
    gic_write_ich_lr_el2(0U, lr0);
    gic_write_ich_hcr_el2(hcr);
    gic_write_ich_ap1r_el2(0U, ap1r0);
    isb();
}

void gic_host_ich_program(uint64_t hcr, uint64_t vmcr, uint64_t lr0)
{
    gic_host_ich_program_with_ap1r0(hcr, vmcr, lr0, 0U);
}

void gic_host_ich_write_ap0r(uint32_t index, uint64_t val)
{
    gic_write_ich_ap0r_el2(index, val);
    isb();
}

void gic_host_ich_write_ap0r0(uint64_t val)
{
    gic_host_ich_write_ap0r(0U, val);
}

void gic_host_ich_write_ap1r(uint32_t index, uint64_t val)
{
    gic_write_ich_ap1r_el2(index, val);
    isb();
}

void gic_host_ich_write_ap1r0(uint64_t val)
{
    gic_host_ich_write_ap1r(0U, val);
}

void gic_host_ich_write_hcr(uint64_t val)
{
    gic_write_ich_hcr_el2(val);
    isb();
}

void gic_host_ich_write_lr(uint32_t index, uint64_t val)
{
    gic_write_ich_lr_el2(index, val);
    isb();
}

void gic_host_ich_write_vmcr(uint64_t val)
{
    gic_write_ich_vmcr_el2(val);
    isb();
}

uint64_t gic_host_ich_read_ap0r(uint32_t index)
{
    return gic_read_ich_ap0r_el2(index);
}

uint64_t gic_host_ich_read_ap0r0(void)
{
    return gic_host_ich_read_ap0r(0U);
}

uint64_t gic_host_ich_read_ap1r(uint32_t index)
{
    return gic_read_ich_ap1r_el2(index);
}

uint64_t gic_host_ich_read_ap1r0(void)
{
    return gic_host_ich_read_ap1r(0U);
}

uint64_t gic_host_ich_read_hcr(void)
{
    return gic_read_ich_hcr_el2();
}

uint32_t gic_host_ich_num_lrs(void)
{
    uint32_t num_lrs;

    num_lrs = (uint32_t)(gic_read_ich_vtr_el2() & 0x1fU) + 1U;
    if (num_lrs > VAL_REC_GIC_NUM_LRS) {
        num_lrs = VAL_REC_GIC_NUM_LRS;
    }

    return num_lrs;
}

uint32_t gic_host_ich_num_aprs(void)
{
    uint32_t prebits;
    uint32_t num_aprs;

    prebits = (uint32_t)((gic_read_ich_vtr_el2() >> GIC_ICH_VTR_PREBITS_SHIFT) &
                         GIC_ICH_VTR_PREBITS_MASK) + 1U;

    if (prebits <= 5U) {
        num_aprs = 1U;
    } else if (prebits == 6U) {
        num_aprs = 2U;
    } else {
        num_aprs = GIC_ICH_MAX_APR_NUM_REGS;
    }

    return num_aprs;
}

uint64_t gic_host_ich_read_lr(uint32_t index)
{
    return gic_read_ich_lr_el2(index);
}

uint64_t gic_host_ich_read_lr0(void)
{
    return gic_host_ich_read_lr(0U);
}

uint64_t gic_host_ich_read_vmcr(void)
{
    return gic_read_ich_vmcr_el2();
}

uint32_t gic_host_expect_sysreg_exit(val_host_realm_ts *realm)
{
    val_host_rec_exit_ts *rec_exit;
    uint64_t ret;

    if ((realm == NULL) || (realm->run[0] == 0UL) || (realm->rec[0] == 0UL)) {
        LOG(ERROR, "Invalid realm context for sysreg exit check\n");
        return VAL_ERROR;
    }

    rec_exit = &(((val_host_rec_run_ts *)realm->run[0])->exit);

    ret = val_host_rmi_rec_enter(realm->rec[0], realm->run[0]);
    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        return VAL_ERROR;
    }

    if ((rec_exit->exit_reason != RMI_EXIT_SYNC) ||
        (ESR_EL2_EC(rec_exit->esr) != ESR_EL2_EC_SYSREG)) {
        LOG(ERROR, "Expected sysreg REC exit, exit_reason=%x esr=%lx\n",
            rec_exit->exit_reason, rec_exit->esr);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}
