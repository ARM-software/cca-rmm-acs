/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef ARCH_HELPERS_H
#define ARCH_HELPERS_H

#include <arch.h>
#include <cdefs.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef unsigned long u_register_t;

#define COMPILER_BARRIER() __asm__ volatile ("" ::: "memory")

/**********************************************************************
 * Macros which create inline functions to read or write CPU system
 * registers
 *********************************************************************/

#define _DEFINE_SYSREG_READ_FUNC(_name, _reg_name)        \
static inline u_register_t read_ ## _name(void)            \
{                                \
    u_register_t v;                        \
    __asm__ volatile ("mrs %0, " #_reg_name : "=r" (v));    \
    return v;                        \
}

#define _DEFINE_SYSREG_WRITE_FUNC(_name, _reg_name)            \
static inline void write_ ## _name(u_register_t v)            \
{                                    \
    __asm__ volatile ("msr " #_reg_name ", %0" : : "r" (v));    \
}

#define SYSREG_WRITE_CONST(reg_name, v)                \
    __asm__ volatile ("msr " #reg_name ", %0" : : "i" (v))

/* Define read function for system register */
#define DEFINE_SYSREG_READ_FUNC(_name)             \
    _DEFINE_SYSREG_READ_FUNC(_name, _name)

/* Define read & write function for system register */
#define DEFINE_SYSREG_RW_FUNCS(_name)            \
    _DEFINE_SYSREG_READ_FUNC(_name, _name)        \
    _DEFINE_SYSREG_WRITE_FUNC(_name, _name)

/* Define read & write function for renamed system register */
#define DEFINE_RENAME_SYSREG_RW_FUNCS(_name, _reg_name)    \
    _DEFINE_SYSREG_READ_FUNC(_name, _reg_name)    \
    _DEFINE_SYSREG_WRITE_FUNC(_name, _reg_name)

/* Define read function for renamed system register */
#define DEFINE_RENAME_SYSREG_READ_FUNC(_name, _reg_name)    \
    _DEFINE_SYSREG_READ_FUNC(_name, _reg_name)

/* Define write function for renamed system register */
#define DEFINE_RENAME_SYSREG_WRITE_FUNC(_name, _reg_name)    \
    _DEFINE_SYSREG_WRITE_FUNC(_name, _reg_name)

/**********************************************************************
 * Macros to create inline functions for system instructions
 *********************************************************************/

/* Define function for simple system instruction */
#define DEFINE_SYSOP_FUNC(_op)                \
static inline void _op(void)                \
{                            \
    __asm__ (#_op);                    \
}

/* Define function for system instruction with type specifier */
#define DEFINE_SYSOP_TYPE_FUNC(_op, _type)        \
static inline void _op ## _type(void)            \
{                            \
    __asm__ (#_op " " #_type);            \
}

/* Define function for system instruction with register parameter */
#define DEFINE_SYSOP_TYPE_PARAM_FUNC(_op, _type)    \
static inline void _op ## _type(uint64_t v)        \
{                            \
     __asm__ (#_op " " #_type ", %0" : : "r" (v));    \
}

/*******************************************************************************
 * TLB maintenance accessor prototypes
 ******************************************************************************/

#if ERRATA_A57_813419
/*
 * Define function for TLBI instruction with type specifier that implements
 * the workaround for errata 813419 of Cortex-A57.
 */
#define DEFINE_TLBIOP_ERRATA_A57_813419_TYPE_FUNC(_type)\
static inline void tlbi ## _type(void)            \
{                            \
    __asm__("tlbi " #_type "\n"            \
        "dsb ish\n"                \
        "tlbi " #_type);            \
}

/*
 * Define function for TLBI instruction with register parameter that implements
 * the workaround for errata 813419 of Cortex-A57.
 */
#define DEFINE_TLBIOP_ERRATA_A57_813419_TYPE_PARAM_FUNC(_type)    \
static inline void tlbi ## _type(uint64_t v)            \
{                                \
    __asm__("tlbi " #_type ", %0\n"                \
        "dsb ish\n"                    \
        "tlbi " #_type ", %0" : : "r" (v));        \
}
#endif /* ERRATA_A57_813419 */

DEFINE_SYSOP_TYPE_FUNC(tlbi, alle1)
DEFINE_SYSOP_TYPE_FUNC(tlbi, alle1is)
DEFINE_SYSOP_TYPE_FUNC(tlbi, alle2)
DEFINE_SYSOP_TYPE_FUNC(tlbi, alle2is)
#if ERRATA_A57_813419
DEFINE_TLBIOP_ERRATA_A57_813419_TYPE_FUNC(alle3)
DEFINE_TLBIOP_ERRATA_A57_813419_TYPE_FUNC(alle3is)
#else
DEFINE_SYSOP_TYPE_FUNC(tlbi, alle3)
DEFINE_SYSOP_TYPE_FUNC(tlbi, alle3is)
#endif
DEFINE_SYSOP_TYPE_FUNC(tlbi, vmalle1)

DEFINE_SYSOP_TYPE_PARAM_FUNC(tlbi, vaae1is)
DEFINE_SYSOP_TYPE_PARAM_FUNC(tlbi, vaale1is)
DEFINE_SYSOP_TYPE_PARAM_FUNC(tlbi, vae2is)
DEFINE_SYSOP_TYPE_PARAM_FUNC(tlbi, vale2is)
#if ERRATA_A57_813419
DEFINE_TLBIOP_ERRATA_A57_813419_TYPE_PARAM_FUNC(vae3is)
DEFINE_TLBIOP_ERRATA_A57_813419_TYPE_PARAM_FUNC(vale3is)
#else
DEFINE_SYSOP_TYPE_PARAM_FUNC(tlbi, vae3is)
DEFINE_SYSOP_TYPE_PARAM_FUNC(tlbi, vale3is)
#endif

/*******************************************************************************
 * Cache maintenance accessor prototypes
 ******************************************************************************/
DEFINE_SYSOP_TYPE_PARAM_FUNC(dc, isw)
DEFINE_SYSOP_TYPE_PARAM_FUNC(dc, cisw)
DEFINE_SYSOP_TYPE_PARAM_FUNC(dc, csw)
DEFINE_SYSOP_TYPE_PARAM_FUNC(dc, cvac)
DEFINE_SYSOP_TYPE_PARAM_FUNC(dc, ivac)
DEFINE_SYSOP_TYPE_PARAM_FUNC(dc, civac)
DEFINE_SYSOP_TYPE_PARAM_FUNC(dc, cvau)
DEFINE_SYSOP_TYPE_PARAM_FUNC(dc, zva)

/*******************************************************************************
 * Address translation accessor prototypes
 ******************************************************************************/
DEFINE_SYSOP_TYPE_PARAM_FUNC(at, s12e1r)
DEFINE_SYSOP_TYPE_PARAM_FUNC(at, s12e1w)
DEFINE_SYSOP_TYPE_PARAM_FUNC(at, s12e0r)
DEFINE_SYSOP_TYPE_PARAM_FUNC(at, s12e0w)
DEFINE_SYSOP_TYPE_PARAM_FUNC(at, s1e1r)
DEFINE_SYSOP_TYPE_PARAM_FUNC(at, s1e2r)
DEFINE_SYSOP_TYPE_PARAM_FUNC(at, s1e3r)

void flush_dcache_range(uintptr_t addr, size_t size);
void clean_dcache_range(uintptr_t addr, size_t size);
void inv_dcache_range(uintptr_t addr, size_t size);

void dcsw_op_louis(u_register_t op_type);
void dcsw_op_all(u_register_t op_type);

void disable_mmu(void);
void disable_mmu_icache(void);

/*******************************************************************************
 * Misc. accessor prototypes
 ******************************************************************************/

#define write_daifclr(val) SYSREG_WRITE_CONST(daifclr, val)
#define write_daifset(val) SYSREG_WRITE_CONST(daifset, val)
DEFINE_SYSOP_FUNC(wfi)
DEFINE_SYSOP_FUNC(wfe)
DEFINE_SYSOP_FUNC(sev)
DEFINE_SYSOP_TYPE_FUNC(dsb, sy)
DEFINE_SYSOP_TYPE_FUNC(dmb, sy)
DEFINE_SYSOP_TYPE_FUNC(dmb, st)
DEFINE_SYSOP_TYPE_FUNC(dmb, ld)
DEFINE_SYSOP_TYPE_FUNC(dsb, ish)
DEFINE_SYSOP_TYPE_FUNC(dsb, nsh)
DEFINE_SYSOP_TYPE_FUNC(dsb, ishst)
DEFINE_SYSOP_TYPE_FUNC(dmb, oshld)
DEFINE_SYSOP_TYPE_FUNC(dmb, oshst)
DEFINE_SYSOP_TYPE_FUNC(dmb, osh)
DEFINE_SYSOP_TYPE_FUNC(dmb, nshld)
DEFINE_SYSOP_TYPE_FUNC(dmb, nshst)
DEFINE_SYSOP_TYPE_FUNC(dmb, nsh)
DEFINE_SYSOP_TYPE_FUNC(dmb, ishld)
DEFINE_SYSOP_TYPE_FUNC(dmb, ishst)
DEFINE_SYSOP_TYPE_FUNC(dmb, ish)
DEFINE_SYSOP_FUNC(isb)

/*******************************************************************************
 * System register accessor prototypes
 ******************************************************************************/
DEFINE_SYSREG_READ_FUNC(midr_el1)
DEFINE_SYSREG_READ_FUNC(mpidr_el1)
DEFINE_SYSREG_RW_FUNCS(vpidr_el2)
DEFINE_SYSREG_RW_FUNCS(vmpidr_el2)

DEFINE_SYSREG_RW_FUNCS(par_el1)

DEFINE_SYSREG_READ_FUNC(id_pfr1_el1)
DEFINE_SYSREG_READ_FUNC(id_aa64isar0_el1)
DEFINE_SYSREG_READ_FUNC(id_aa64isar1_el1)
DEFINE_SYSREG_READ_FUNC(id_aa64pfr0_el1)
DEFINE_SYSREG_READ_FUNC(id_aa64pfr1_el1)
DEFINE_SYSREG_READ_FUNC(id_aa64dfr0_el1)
DEFINE_SYSREG_READ_FUNC(id_afr0_el1)
DEFINE_SYSREG_READ_FUNC(id_aa64mmfr0_el1)
DEFINE_SYSREG_READ_FUNC(id_aa64mmfr1_el1)

DEFINE_RENAME_SYSREG_READ_FUNC(id_aa64smfr0_el1, ID_AA64SMFR0_EL1)

DEFINE_SYSREG_READ_FUNC(CurrentEl)
DEFINE_SYSREG_READ_FUNC(ctr_el0)
DEFINE_SYSREG_RW_FUNCS(daif)
DEFINE_SYSREG_RW_FUNCS(nzcv)
DEFINE_SYSREG_READ_FUNC(spsel)

DEFINE_SYSREG_RW_FUNCS(spsr_el1)
DEFINE_SYSREG_RW_FUNCS(spsr_el2)
DEFINE_SYSREG_RW_FUNCS(spsr_el3)

DEFINE_SYSREG_RW_FUNCS(elr_el1)
DEFINE_SYSREG_RW_FUNCS(elr_el2)
DEFINE_SYSREG_RW_FUNCS(elr_el3)

DEFINE_SYSREG_RW_FUNCS(scr_el3)
DEFINE_SYSREG_RW_FUNCS(hcr_el2)

DEFINE_SYSREG_RW_FUNCS(vbar_el1)
DEFINE_SYSREG_RW_FUNCS(vbar_el2)
DEFINE_SYSREG_RW_FUNCS(vbar_el3)

DEFINE_SYSREG_RW_FUNCS(sctlr_el1)
DEFINE_SYSREG_RW_FUNCS(sctlr_el2)
DEFINE_SYSREG_RW_FUNCS(sctlr_el3)

DEFINE_SYSREG_RW_FUNCS(esr_el1)
DEFINE_SYSREG_RW_FUNCS(esr_el2)
DEFINE_SYSREG_RW_FUNCS(esr_el3)

DEFINE_SYSREG_RW_FUNCS(far_el1)
DEFINE_SYSREG_RW_FUNCS(far_el2)
DEFINE_SYSREG_RW_FUNCS(far_el3)

DEFINE_SYSREG_RW_FUNCS(mair_el1)
DEFINE_SYSREG_RW_FUNCS(mair_el2)
DEFINE_SYSREG_RW_FUNCS(mair_el3)

DEFINE_SYSREG_RW_FUNCS(amair_el1)
DEFINE_SYSREG_RW_FUNCS(amair_el2)
DEFINE_SYSREG_RW_FUNCS(amair_el3)

DEFINE_SYSREG_RW_FUNCS(tcr_el1)
DEFINE_SYSREG_RW_FUNCS(tcr_el2)
DEFINE_SYSREG_RW_FUNCS(tcr_el3)

DEFINE_SYSREG_RW_FUNCS(ttbr0_el1)
DEFINE_SYSREG_RW_FUNCS(ttbr0_el2)
DEFINE_SYSREG_RW_FUNCS(ttbr0_el3)

DEFINE_SYSREG_RW_FUNCS(ttbr1_el1)
DEFINE_SYSREG_RW_FUNCS(vttbr_el2)

DEFINE_RENAME_SYSREG_RW_FUNCS(svcr, SVCR)
DEFINE_RENAME_SYSREG_RW_FUNCS(tpidr2_el0, TPIDR2_EL0)
DEFINE_RENAME_SYSREG_RW_FUNCS(smcr_el2, SMCR_EL2)

/* Armv8.1 Registers */
DEFINE_RENAME_SYSREG_RW_FUNCS(pan, PAN)

/* Armv8.2 Registers */
DEFINE_RENAME_SYSREG_READ_FUNC(id_aa64mmfr2_el1, ID_AA64MMFR2_EL1)

#define IS_IN_EL(x) \
    (GET_EL(read_CurrentEl()) == MODE_EL##x)

#define IS_IN_EL1() IS_IN_EL(1)
#define IS_IN_EL2() IS_IN_EL(2)
#define IS_IN_EL3() IS_IN_EL(3)

static inline unsigned int get_current_el(void)
{
    return GET_EL(read_CurrentEl());
}

/*
 * Check if an EL is implemented from AA64PFR0 register fields.
 */
static inline uint64_t el_implemented(unsigned int el)
{
    if (el > 3U) {
        return EL_IMPL_NONE;
    } else {
        unsigned int shift = ID_AA64PFR0_EL1_SHIFT * el;

        return (read_id_aa64pfr0_el1() >> shift) & ID_AA64PFR0_ELX_MASK;
    }
}

#endif /* ARCH_HELPERS_H */
