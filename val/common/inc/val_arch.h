/*
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_ARCH_H_
#define _VAL_ARCH_H_

#define EL2 2
#define EL1 1

#define SCTLR_I_BIT         (1 << 12)
#define SCTLR_M_BIT         (1 << 0)

#define EXTRACT_BIT(regfield, reg) \
    (((reg) >> (regfield##_SHIFT)) & 1ul)

#define ESR_EL2_EC_SHIFT    26
#define ESR_EL2_EC_WIDTH    6
#define ESR_EL2_EC_MASK        MASK(ESR_EL2_EC)
#define ESR_EL2_IL_SHIFT    25
#define ESR_EL2_IL_WIDTH    1
#define ESR_EL2_IL_MASK        MASK(ESR_EL2_EC)

#define ESR_EL2_ISS_SHIFT 0
#define ESR_EL2_ISS_WIDTH 25
#define ESR_EL2_ISS_MASK  MASK(ESR_EL2_ISS)

#define ESR_EL2_EC(val) (val & MASK(ESR_EL2_EC))

#define ESR_EL2_EC_WFX                  INPLACE(ESR_EL2_EC, 1)
#define ESR_EL2_EC_SVC                  INPLACE(ESR_EL2_EC, 21)
#define ESR_EL2_EC_HVC                  INPLACE(ESR_EL2_EC, 22)
#define ESR_EL2_EC_SMC                  INPLACE(ESR_EL2_EC, 23)
#define ESR_EL2_EC_SYSREG               INPLACE(ESR_EL2_EC, 24)
#define ESR_EL2_EC_INST_ABORT           INPLACE(ESR_EL2_EC, 32)
#define ESR_EL2_EC_DATA_ABORT           INPLACE(ESR_EL2_EC, 36)
#define ESR_EL2_EC_DATA_ABORT_SEL       INPLACE(ESR_EL2_EC, 37)
#define ESR_EL2_EC_FPU                  INPLACE(ESR_EL2_EC, 0x7)

#define ESR_EL2_WFX_TI(val) (val & MASK(ESR_EL2_WFX_TI))
#define ESR_EL2_ABORT_FSC(val) (val & MASK(ESR_EL2_ABORT_FSC))

#define ESR_EL2_WFX_TI_WFI      INPLACE(ESR_EL2_WFX_TI, 0x0)
#define ESR_EL2_WFX_TI_WFE      INPLACE(ESR_EL2_WFX_TI, 0x1)


#define ESR_EL2_WFX_TI_SHIFT  0
#define ESR_EL2_WFX_TI_WIDTH  2

/* Data/Instruction Abort ESR fields */
#define ESR_EL2_ABORT_ISV_BIT        (1UL << 24)

#define ESR_EL2_ABORT_SAS_SHIFT        22
#define ESR_EL2_ABORT_SAS_WIDTH        2
#define ESR_EL2_ABORT_SAS_MASK        MASK(ESR_EL2_ABORT_SAS)

#define ESR_EL2_ABORT_SAS_BYTE_VAL    0
#define ESR_EL2_ABORT_SAS_HWORD_VAL    1
#define ESR_EL2_ABORT_SAS_WORD_VAL    2
#define ESR_EL2_ABORT_SAS_DWORD_VAL    3

#define ESR_EL2_ABORT_SSE_BIT        (1UL << 21)

#define ESR_EL2_ABORT_SRT_SHIFT        16
#define ESR_EL2_ABORT_SRT_WIDTH        5
#define ESR_EL2_ABORT_SRT_MASK        MASK(ESR_EL2_ABORT_SRT)

#define ESR_EL2_ABORT_SF_BIT        (1UL << 15)
#define ESR_EL2_ABORT_FNV_BIT        (1UL << 10)
#define ESR_EL2_ABORT_WNR_BIT        (1UL << 6)
#define ESR_EL2_ABORT_FSC_SHIFT        0
#define ESR_EL2_ABORT_FSC_WIDTH        6
#define ESR_EL2_ABORT_FSC_MASK        MASK(ESR_EL2_ABORT_FSC)

#define ESR_EL2_ABORT_FSC_TRANSLATION_FAULT      0x04
#define ESR_EL2_ABORT_FSC_PERMISSION_FAULT       0x0c
#define ESR_EL2_ABORT_FSC_PERMISSION_FAULT_L3    0x0f
#define ESR_EL2_ABORT_FSC_LEVEL_SHIFT        0
#define ESR_EL2_ABORT_FSC_LEVEL_WIDTH        2
#define ESR_EL2_ABORT_FSC_LEVEL_MASK        MASK(ESR_EL2_ABORT_FSC_LEVEL)
#define ESR_EL2_ABORT_FSC_GPF            0x28

#define ESR_EL2_SYSREG_TRAP_OP0_SHIFT    20
#define ESR_EL2_SYSREG_TRAP_OP0_WIDTH    2
#define ESR_EL2_SYSREG_TRAP_OP0_MASK    MASK(ESR_EL2_SYSREG_TRAP_OP0)

#define ESR_EL2_SYSREG_TRAP_OP2_SHIFT    17
#define ESR_EL2_SYSREG_TRAP_OP2_WIDTH    3
#define ESR_EL2_SYSREG_TRAP_OP2_MASK    MASK(ESR_EL2_SYSREG_TRAP_OP2)

#define ESR_EL2_SYSREG_TRAP_OP1_SHIFT    14
#define ESR_EL2_SYSREG_TRAP_OP1_WIDTH    3
#define ESR_EL2_SYSREG_TRAP_OP1_MASK    MASK(ESR_EL2_SYSREG_TRAP_OP1)

#define ESR_EL2_SYSREG_TRAP_CRN_SHIFT    10
#define ESR_EL2_SYSREG_TRAP_CRN_WIDTH    4
#define ESR_EL2_SYSREG_TRAP_CRN_MASK    MASK(ESR_EL2_SYSREG_TRAP_CRN)

#define ESR_EL2_SYSREG_TRAP_RT_SHIFT    5
#define ESR_EL2_SYSREG_TRAP_RT_WIDTH    5
#define ESR_EL2_SYSREG_TRAP_RT_MASK    MASK(ESR_EL2_SYSREG_TRAP_RT)

#define ESR_EL2_SYSREG_TRAP_CRM_SHIFT    1
#define ESR_EL2_SYSREG_TRAP_CRM_WIDTH    4
#define ESR_EL2_SYSREG_TRAP_CRM_MASK    MASK(ESR_EL2_SYSREG_TRAP_CRM)

/* WFx ESR fields */
#define ESR_EL2_WFx_TI_BIT        (1UL << 0)

/* xVC ESR fields */
#define ESR_EL2_xVC_IMM_SHIFT        0
#define ESR_EL2_xVC_IMM_WIDTH        16
#define ESR_EL2_xVC_IMM_MASK        MASK(ESR_EL2_xVC_IMM)

/* System Register encodings */
#define SYSREG_ID_OP0_SHIFT	19
#define SYSREG_ID_OP1_SHIFT	16
#define SYSREG_ID_CRN_SHIFT	12
#define SYSREG_ID_CRM_SHIFT	8
#define SYSREG_ID_OP2_SHIFT	5

#define SYSREG_ID(op0, op1, crn, crm, op2) \
		((UL(op0) << SYSREG_ID_OP0_SHIFT) | \
		 (UL(op1) << SYSREG_ID_OP1_SHIFT) | \
		 (UL(crn) << SYSREG_ID_CRN_SHIFT) | \
		 (UL(crm) << SYSREG_ID_CRM_SHIFT) | \
		 (UL(op2) << SYSREG_ID_OP2_SHIFT))

#define SYSREG_SCTLR_EL1			SYSREG_ID(3, 0, 1, 0, 0)
#define SYSREG_SCTLR_EL3			SYSREG_ID(3, 3, 1, 0, 0)
#define SYSREG_PMCR_EL0      		SYSREG_ID(3, 3, 9, 12, 0)


#endif /* _VAL_ARCH_H_ */
