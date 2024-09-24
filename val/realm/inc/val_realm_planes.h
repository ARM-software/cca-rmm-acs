/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_REALM_PLANES_H_
#define _VAL_REALM_PLANES_H_

#include "val_realm_rsi.h"

#define SET_MEMBER_RSI SET_MEMBER
#define VAL_PLANE_ENTRY_GPRS                  31
#define VAL_PLANE_EXIT_GPRS                   31
#define VAL_PLANE_GIC_NUM_LRS                 16

#define PLANE_1_INDEX       1

#define PLANE_1_PERMISSION_INDEX              2

#define PSI_RETURN_TO_PN    1
#define PSI_RETURN_TO_P0    2

/* Base & overlay permissions labels */
#define S2_AP_NO_ACCESS     0
#define S2_AP_RESERVED_1    1
#define S2_AP_MR0           2
#define S2_AP_MR0_TL1       3
#define S2_AP_WO            4
#define S2_AP_RESERVED_5    5
#define S2_AP_MR0_TL0       6
#define S2_AP_MR0_TL01      7
#define S2_AP_RO            8
#define S2_AP_RO_uX         9
#define S2_AP_RO_pX         10
#define S2_AP_RO_upX        11
#define S2_AP_RW            12
#define S2_AP_RW_uX         13
#define S2_AP_RW_pX         14
#define S2_AP_RW_upX        15

typedef struct {
    uint64_t sp_el0;           /*   0x0 */
    uint64_t sp_el1;           /*   0x8 */
    uint64_t elr_el1;          /*  0x10 */
    uint64_t spsr_el1;         /*  0x18 */
    uint64_t pmcr_el0;         /*  0x20 */
    uint64_t pmuserenr_el0;    /*  0x28 */
    uint64_t tpidrro_el0;      /*  0x30 */
    uint64_t tpidr_el0;        /*  0x38 */
    uint64_t csselr_el1;       /*  0x40 */
    uint64_t sctlr_el1;        /*  0x48 */
    uint64_t actlr_el1;        /*  0x50 */
    uint64_t cpacr_el1;        /*  0x58 */
    uint64_t zcr_el1;          /*  0x60 */
    uint64_t ttbr0_el1;        /*  0x68 */
    uint64_t ttbr1_el1;        /*  0x70 */
    uint64_t tcr_el1;          /*  0x78 */
    uint64_t esr_el1;          /*  0x80 */
    uint64_t afsr0_el1;        /*  0x88 */
    uint64_t afsr1_el1;        /*  0x90 */
    uint64_t far_el1;          /*  0x98 */
    uint64_t mair_el1;         /*  0xA0 */
    uint64_t vbar_el1;         /*  0xA8 */
    uint64_t contextidr_el1;   /*  0xB0 */
    uint64_t tpidr_el1;        /*  0xB8 */
    uint64_t amair_el1;        /*  0xC0 */
    uint64_t cntkctl_el1;      /*  0xC8 */
    uint64_t par_el1;          /*  0xD0 */
    uint64_t mdscr_el1;        /*  0xD8 */
    uint64_t mdccint_el1;      /*  0xE0 */
    uint64_t disr_el1;         /*  0xE8 */
    uint64_t mpam0_el1;        /*  0xF0 */

    /* Timer Registers */
    uint64_t cntp_ctl_el0;  /*  0xF8 */
    uint64_t cntp_cval_el0;  /* 0x100 */
    uint64_t cntv_ctl_el0;  /* 0x108 */
    uint64_t cntv_cval_el0;  /* 0x110 */
} val_realm_rsi_plane_el1_sysregs;

typedef struct __attribute__((packed)) {
    uint8_t trap_wfi:1;
    uint8_t trap_wfe:1;
    uint8_t trap_hc:1;
    uint8_t gic_owner:1;
    uint64_t unused:60;
} val_realm_plane_enter_flags_ts;

typedef struct {
    /* Flags */
    SET_MEMBER_RSI(uint64_t flags, 0, 0x8); /* Offset 0 */
    /* Pc */
    SET_MEMBER_RSI(uint64_t pc, 0x8, 0x100); /* 0x8 */
    /* General-purpose registers */
    SET_MEMBER_RSI(uint64_t gprs[VAL_PLANE_ENTRY_GPRS], 0x100, 0x200); /* 0x100 */
    /* EL1 system registers */
    SET_MEMBER_RSI(struct {
            /* GICv3 Hypervisor Control Register */
            uint64_t gicv3_hcr;   /* 0x200 */
            /* GICv3 List Registers */
            uint64_t gicv3_lrs[VAL_PLANE_GIC_NUM_LRS]; /* 0x208 */
            }, 0x200, 0x300);
} val_realm_plane_enter_ts;

typedef struct {
    /* Exit reason */
    SET_MEMBER_RSI(uint64_t reason, 0, 0x100);/* Offset 0 */
    SET_MEMBER_RSI(struct {
            /* Exception Syndrome Register */
            uint64_t elr_el2;  /* 0x100 */
            /* Exception Syndrome Register */
            uint64_t esr_el2;  /* 0x108 */
            /* Fault Address Register */
            uint64_t far_el2;  /* 0x110 */
            /* Hypervisor IPA Fault Address register */
            uint64_t hpfar_el2;  /* 0x118 */
            }, 0x100, 0x200);
    /* General-purpose registers */
    SET_MEMBER_RSI(uint64_t gprs[VAL_PLANE_EXIT_GPRS], 0x200, 0x300); /* 0x200 */
    /* EL1 system registers */
    SET_MEMBER_RSI(struct {
            /* GICv3 Hypervisor Control Register */
            uint64_t gicv3_hcr; /* 0x300 */
            /* GICv3 List Registers */
            uint64_t gicv3_lrs[VAL_PLANE_GIC_NUM_LRS]; /* 0x308 */
            /* GICv3 Maintenance Interrupt State Register */
            uint64_t gicv3_misr; /* 0x388 */
            /* GICv3 Virtual Machine Control Register */
            uint64_t gicv3_vmcr; /* 0x390 */
            }, 0x300, 0x400);
    SET_MEMBER_RSI(struct {
            /* Counter-timer Physical Timer Control Register Value */
            uint64_t cntp_ctl; /* 0x400 */
            /* GICv3 List Registers */
            /* Counter-timer Physical Timer Compare Value */
            uint64_t cntp_cval; /* 0x408 */
            /* Counter-timer Vitual Timer Control Register Value */
            uint64_t cntv_ctl; /* 0x410 */
            /* Counter-timer Vitual Timer Compare Value */
            uint64_t cntv_cval; /* 0x418 */
            }, 0x400, 0x500);

} val_realm_plane_exit_ts;

typedef struct {
    /* Entry information */
    SET_MEMBER_RSI(val_realm_plane_enter_ts enter, 0, 0x800); /* Offset 0 */
    /* Exit information */
    SET_MEMBER_RSI(val_realm_plane_exit_ts exit, 0x800, 0x1000);/* 0x800 */
} val_realm_rsi_plane_run_ts;

uint64_t val_realm_psi_realm_config(uint64_t buff);
uint64_t val_realm_plane_handle_exception(val_realm_rsi_plane_run_ts *run);
uint64_t hvc_handle_realm_config(val_realm_rsi_plane_run_ts *run);
uint64_t hvc_handle_realm_print(void);
void val_realm_preserve_plane_context(val_realm_rsi_plane_run_ts *run);
void val_realm_return_to_p0(void);
uint64_t val_realm_run_plane(uint64_t plane_index, val_realm_rsi_plane_run_ts *run_ptr);
uint64_t val_realm_plane_perm_init(uint64_t plane_idx, uint64_t perm_idx,
                                                       uint64_t base, uint64_t top);

#endif /* _VAL_REALM_PLANES_H_ */
