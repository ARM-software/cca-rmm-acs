/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _ARCH_H_
#define _ARCH_H_

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
#define SYSREG_ID_OP0_SHIFT    19
#define SYSREG_ID_OP1_SHIFT    16
#define SYSREG_ID_CRN_SHIFT    12
#define SYSREG_ID_CRM_SHIFT    8
#define SYSREG_ID_OP2_SHIFT    5

#define SYSREG_ID(op0, op1, crn, crm, op2) \
        ((UL(op0) << SYSREG_ID_OP0_SHIFT) | \
         (UL(op1) << SYSREG_ID_OP1_SHIFT) | \
         (UL(crn) << SYSREG_ID_CRN_SHIFT) | \
         (UL(crm) << SYSREG_ID_CRM_SHIFT) | \
         (UL(op2) << SYSREG_ID_OP2_SHIFT))

#define SYSREG_SCTLR_EL1            SYSREG_ID(3, 0, 1, 0, 0)
#define SYSREG_SCTLR_EL3            SYSREG_ID(3, 3, 1, 0, 0)
#define SYSREG_PMCR_EL0              SYSREG_ID(3, 3, 9, 12, 0)

/*******************************************************************************
 * MIDR bit definitions
 ******************************************************************************/
#define MIDR_IMPL_MASK        U(0xff)
#define MIDR_IMPL_SHIFT        U(0x18)
#define MIDR_VAR_SHIFT        U(20)
#define MIDR_VAR_BITS        U(4)
#define MIDR_VAR_MASK        U(0xf)
#define MIDR_REV_SHIFT        U(0)
#define MIDR_REV_BITS        U(4)
#define MIDR_REV_MASK        U(0xf)
#define MIDR_PN_MASK        U(0xfff)
#define MIDR_PN_SHIFT        U(0x4)

/*******************************************************************************
 * MPIDR macros
 ******************************************************************************/
#define MPIDR_MT_MASK        (ULL(1) << 24)
#define MPIDR_CPU_MASK        MPIDR_AFFLVL_MASK
#define MPIDR_CLUSTER_MASK    (MPIDR_AFFLVL_MASK << MPIDR_AFFINITY_BITS)
#define MPIDR_AFFINITY_BITS    U(8)
#define MPIDR_AFFLVL_MASK    ULL(0xff)
#define MPIDR_AFF0_SHIFT    0
#define MPIDR_AFF1_SHIFT    U(8)
#define MPIDR_AFF2_SHIFT    U(16)
#define MPIDR_AFF3_SHIFT    U(32)
#define MPIDR_AFF_SHIFT(_n)    MPIDR_AFF##_n##_SHIFT
//#define MPIDR_AFFINITY_MASK    ULL(0xff00ffffff)
#define MPIDR_AFFLVL_SHIFT    U(3)
#define MPIDR_AFFLVL0        ULL(0x0)
#define MPIDR_AFFLVL1        ULL(0x1)
#define MPIDR_AFFLVL2        ULL(0x2)
#define MPIDR_AFFLVL3        ULL(0x3)
#define MPIDR_AFFLVL(_n)    MPIDR_AFFLVL##_n
#define MPIDR_AFFLVL0_VAL(mpidr) \
        (((mpidr) >> MPIDR_AFF0_SHIFT) & MPIDR_AFFLVL_MASK)
#define MPIDR_AFFLVL1_VAL(mpidr) \
        (((mpidr) >> MPIDR_AFF1_SHIFT) & MPIDR_AFFLVL_MASK)
#define MPIDR_AFFLVL2_VAL(mpidr) \
        (((mpidr) >> MPIDR_AFF2_SHIFT) & MPIDR_AFFLVL_MASK)
#define MPIDR_AFFLVL3_VAL(mpidr) \
        (((mpidr) >> MPIDR_AFF3_SHIFT) & MPIDR_AFFLVL_MASK)
/*
 * The MPIDR_MAX_AFFLVL count starts from 0. Take care to
 * add one while using this macro to define array sizes.
 */
#define MPIDR_MAX_AFFLVL    U(2)

#define MPID_MASK        (MPIDR_MT_MASK                 | \
                 (MPIDR_AFFLVL_MASK << MPIDR_AFF3_SHIFT) | \
                 (MPIDR_AFFLVL_MASK << MPIDR_AFF2_SHIFT) | \
                 (MPIDR_AFFLVL_MASK << MPIDR_AFF1_SHIFT) | \
                 (MPIDR_AFFLVL_MASK << MPIDR_AFF0_SHIFT))

#define MPIDR_AFF_ID(mpid, n)                    \
    (((mpid) >> MPIDR_AFF_SHIFT(n)) & MPIDR_AFFLVL_MASK)


#define MPIDR_AFFINITY_MASK ((MPIDR_AFFLVL_MASK << MPIDR_AFF3_SHIFT) | \
                 (MPIDR_AFFLVL_MASK << MPIDR_AFF2_SHIFT) | \
                 (MPIDR_AFFLVL_MASK << MPIDR_AFF1_SHIFT) | \
                 (MPIDR_AFFLVL_MASK << MPIDR_AFF0_SHIFT))

/*
 * An invalid MPID. This value can be used by functions that return an MPID to
 * indicate an error.
 */
#define INVALID_MPID        U(0xFFFFFFFF)

/*******************************************************************************
 * System register bit definitions
 ******************************************************************************/
/* CLIDR definitions */
#define LOUIS_SHIFT        U(21)
#define LOC_SHIFT        U(24)
#define CLIDR_FIELD_WIDTH    U(3)

/* CSSELR definitions */
#define LEVEL_SHIFT        U(1)

/* Data cache set/way op type defines */
#define DCISW            U(0x0)
#define DCCISW            U(0x1)
#define DCCSW            U(0x2)

/* ID_AA64PFR0_EL1 definitions */
#define ID_AA64PFR0_EL0_SHIFT    U(0)
#define ID_AA64PFR0_EL1_SHIFT    U(4)
#define ID_AA64PFR0_EL2_SHIFT    U(8)
#define ID_AA64PFR0_EL3_SHIFT    U(12)
#define ID_AA64PFR0_AMU_SHIFT    U(44)
#define ID_AA64PFR0_AMU_LENGTH    U(4)
#define ID_AA64PFR0_AMU_MASK    ULL(0xf)
#define ID_AA64PFR0_AMU_NOT_SUPPORTED    U(0x0)
#define ID_AA64PFR0_AMU_V1    U(0x1)
#define ID_AA64PFR0_AMU_V1P1    U(0x2)
#define ID_AA64PFR0_ELX_MASK    ULL(0xf)
#define ID_AA64PFR0_SVE_SHIFT    U(32)
#define ID_AA64PFR0_SVE_MASK    ULL(0xf)
#define ID_AA64PFR0_SVE_LENGTH    U(4)
#define ID_AA64PFR0_MPAM_SHIFT    U(40)
#define ID_AA64PFR0_MPAM_MASK    ULL(0xf)
#define ID_AA64PFR0_DIT_SHIFT    U(48)
#define ID_AA64PFR0_DIT_MASK    ULL(0xf)
#define ID_AA64PFR0_DIT_LENGTH    U(4)
#define ID_AA64PFR0_DIT_SUPPORTED    U(1)
#define ID_AA64PFR0_CSV2_SHIFT    U(56)
#define ID_AA64PFR0_CSV2_MASK    ULL(0xf)
#define ID_AA64PFR0_CSV2_LENGTH    U(4)
#define ID_AA64PFR0_FEAT_RME_SHIFT        U(52)
#define ID_AA64PFR0_FEAT_RME_MASK        ULL(0xf)
#define ID_AA64PFR0_FEAT_RME_LENGTH        U(4)
#define ID_AA64PFR0_FEAT_RME_NOT_SUPPORTED    U(0)
#define ID_AA64PFR0_FEAT_RME_V1            U(1)

/* ID_AA64DFR0_EL1.PMS definitions (for ARMv8.2+) */
#define ID_AA64DFR0_PMS_SHIFT    U(32)
#define ID_AA64DFR0_PMS_LENGTH    U(4)
#define ID_AA64DFR0_PMS_MASK    ULL(0xf)

/* ID_AA64DFR0_EL1.DEBUG definitions */
#define ID_AA64DFR0_DEBUG_SHIFT            U(0)
#define ID_AA64DFR0_DEBUG_LENGTH        U(4)
#define ID_AA64DFR0_DEBUG_MASK            ULL(0xf)
#define ID_AA64DFR0_DEBUG_BITS            (ID_AA64DFR0_DEBUG_MASK << \
                         ID_AA64DFR0_DEBUG_SHIFT)
#define ID_AA64DFR0_V8_DEBUG_ARCH_SUPPORTED    U(6)
#define ID_AA64DFR0_V8_DEBUG_ARCH_VHE_SUPPORTED    U(7)
#define ID_AA64DFR0_V8_2_DEBUG_ARCH_SUPPORTED    U(8)
#define ID_AA64DFR0_V8_4_DEBUG_ARCH_SUPPORTED    U(9)

/* ID_AA64DFR0_EL1.TraceBuffer definitions */
#define ID_AA64DFR0_TRACEBUFFER_SHIFT        U(44)
#define ID_AA64DFR0_TRACEBUFFER_MASK        ULL(0xf)
#define ID_AA64DFR0_TRACEBUFFER_SUPPORTED    ULL(1)

/* ID_DFR0_EL1.Tracefilt definitions */
#define ID_AA64DFR0_TRACEFILT_SHIFT        U(40)
#define ID_AA64DFR0_TRACEFILT_MASK        U(0xf)
#define ID_AA64DFR0_TRACEFILT_SUPPORTED        U(1)

/* ID_AA64DFR0_EL1.TraceVer definitions */
#define ID_AA64DFR0_TRACEVER_SHIFT        U(4)
#define ID_AA64DFR0_TRACEVER_MASK        ULL(0xf)
#define ID_AA64DFR0_TRACEVER_SUPPORTED        ULL(1)

#define EL_IMPL_NONE        ULL(0)
#define EL_IMPL_A64ONLY        ULL(1)
#define EL_IMPL_A64_A32        ULL(2)

#define ID_AA64PFR0_GIC_SHIFT    U(24)
#define ID_AA64PFR0_GIC_WIDTH    U(4)
#define ID_AA64PFR0_GIC_MASK    ULL(0xf)

/* ID_AA64ISAR1_EL1 definitions */
#define ID_AA64ISAR1_EL1    S3_0_C0_C6_1
#define ID_AA64ISAR1_GPI_SHIFT    U(28)
#define ID_AA64ISAR1_GPI_WIDTH    U(4)
#define ID_AA64ISAR1_GPI_MASK    ULL(0xf)
#define ID_AA64ISAR1_GPA_SHIFT    U(24)
#define ID_AA64ISAR1_GPA_WIDTH    U(4)
#define ID_AA64ISAR1_GPA_MASK    ULL(0xf)
#define ID_AA64ISAR1_API_SHIFT    U(8)
#define ID_AA64ISAR1_API_WIDTH    U(4)
#define ID_AA64ISAR1_API_MASK    ULL(0xf)
#define ID_AA64ISAR1_APA_SHIFT    U(4)
#define ID_AA64ISAR1_APA_WIDTH    U(4)
#define ID_AA64ISAR1_APA_MASK    ULL(0xf)

/* ID_AA64MMFR0_EL1 definitions */
#define ID_AA64MMFR0_EL1_PARANGE_SHIFT    U(0)
#define ID_AA64MMFR0_EL1_PARANGE_MASK    ULL(0xf)

#define PARANGE_0000    U(32)
#define PARANGE_0001    U(36)
#define PARANGE_0010    U(40)
#define PARANGE_0011    U(42)
#define PARANGE_0100    U(44)
#define PARANGE_0101    U(48)
#define PARANGE_0110    U(52)

#define ID_AA64MMFR0_EL1_ECV_SHIFT         U(60)
#define ID_AA64MMFR0_EL1_ECV_MASK          ULL(0xf)
#define ID_AA64MMFR0_EL1_ECV_NOT_SUPPORTED ULL(0x0)
#define ID_AA64MMFR0_EL1_ECV_SUPPORTED     ULL(0x1)
#define ID_AA64MMFR0_EL1_ECV_SELF_SYNCH    ULL(0x2)

#define ID_AA64MMFR0_EL1_FGT_SHIFT        U(56)
#define ID_AA64MMFR0_EL1_FGT_MASK        ULL(0xf)
#define ID_AA64MMFR0_EL1_FGT_NOT_SUPPORTED    ULL(0x0)
#define ID_AA64MMFR0_EL1_FGT_SUPPORTED        ULL(0x1)

#define ID_AA64MMFR0_EL1_TGRAN4_SHIFT        U(28)
#define ID_AA64MMFR0_EL1_TGRAN4_MASK        ULL(0xf)
#define ID_AA64MMFR0_EL1_TGRAN4_SUPPORTED    ULL(0x0)
#define ID_AA64MMFR0_EL1_TGRAN4_NOT_SUPPORTED    ULL(0xf)

#define ID_AA64MMFR0_EL1_TGRAN64_SHIFT        U(24)
#define ID_AA64MMFR0_EL1_TGRAN64_MASK        ULL(0xf)
#define ID_AA64MMFR0_EL1_TGRAN64_SUPPORTED    ULL(0x0)
#define ID_AA64MMFR0_EL1_TGRAN64_NOT_SUPPORTED    ULL(0xf)

#define ID_AA64MMFR0_EL1_TGRAN16_SHIFT        U(20)
#define ID_AA64MMFR0_EL1_TGRAN16_MASK        ULL(0xf)
#define ID_AA64MMFR0_EL1_TGRAN16_SUPPORTED    ULL(0x1)
#define ID_AA64MMFR0_EL1_TGRAN16_NOT_SUPPORTED    ULL(0x0)

/* ID_AA64MMFR1_EL1 definitions */
#define ID_AA64MMFR1_EL1_PAN_SHIFT        U(20)
#define ID_AA64MMFR1_EL1_PAN_MASK        ULL(0xf)
#define ID_AA64MMFR1_EL1_PAN_NOT_SUPPORTED    ULL(0x0)
#define ID_AA64MMFR1_EL1_PAN_SUPPORTED        ULL(0x1)
#define ID_AA64MMFR1_EL1_PAN2_SUPPORTED        ULL(0x2)
#define ID_AA64MMFR1_EL1_PAN3_SUPPORTED        ULL(0x3)
#define ID_AA64MMFR1_EL1_HCX_SHIFT        U(40)
#define ID_AA64MMFR1_EL1_HCX_MASK        ULL(0xf)
#define ID_AA64MMFR1_EL1_HCX_SUPPORTED        ULL(0x1)
#define ID_AA64MMFR1_EL1_HCX_NOT_SUPPORTED    ULL(0x0)

/* ID_AA64MMFR2_EL1 definitions */
#define ID_AA64MMFR2_EL1        S3_0_C0_C7_2

#define ID_AA64MMFR2_EL1_ST_SHIFT    U(28)
#define ID_AA64MMFR2_EL1_ST_MASK    ULL(0xf)

#define ID_AA64MMFR2_EL1_CNP_SHIFT    U(0)
#define ID_AA64MMFR2_EL1_CNP_MASK    ULL(0xf)

#define ID_AA64MMFR2_EL1_CCIDX_SHIFT        U(20)
#define ID_AA64MMFR2_EL1_CCIDX_MASK        ULL(0xf)
#define ID_AA64MMFR2_EL1_CCIDX_LENGTH        U(4)



/* ID_AA64PFR1_EL1 definitions */
#define ID_AA64PFR1_EL1_SSBS_SHIFT    U(4)
#define ID_AA64PFR1_EL1_SSBS_MASK    ULL(0xf)

#define SSBS_UNAVAILABLE    ULL(0)    /* No architectural SSBS support */

#define ID_AA64PFR1_EL1_BT_SHIFT    U(0)
#define ID_AA64PFR1_EL1_BT_MASK        ULL(0xf)

#define BTI_IMPLEMENTED        ULL(1)    /* The BTI mechanism is implemented */

#define ID_AA64PFR1_EL1_MTE_SHIFT    U(8)
#define ID_AA64PFR1_EL1_MTE_MASK    ULL(0xf)

#define MTE_UNIMPLEMENTED    ULL(0)
#define MTE_IMPLEMENTED_EL0    ULL(1)    /* MTE is only implemented at EL0 */
#define MTE_IMPLEMENTED_ELX    ULL(2)    /* MTE is implemented at all ELs */

#define ID_AA64PFR1_EL1_SME_SHIFT    U(24)
#define ID_AA64PFR1_EL1_SME_MASK    ULL(0xf)

/* ID_PFR1_EL1 definitions */
#define ID_PFR1_VIRTEXT_SHIFT    U(12)
#define ID_PFR1_VIRTEXT_MASK    U(0xf)
#define GET_VIRT_EXT(id)    (((id) >> ID_PFR1_VIRTEXT_SHIFT) \
                 & ID_PFR1_VIRTEXT_MASK)

/* SCTLR definitions */
#define SCTLR_EL2_RES1    ((U(1) << 29) | (U(1) << 28) | (U(1) << 23) | \
             (U(1) << 22) | (U(1) << 18) | (U(1) << 16) | \
             (U(1) << 11) | (U(1) << 5) | (U(1) << 4))

#define SCTLR_EL1_RES1    ((U(1) << 29) | (U(1) << 28) | (U(1) << 23) | \
             (U(1) << 22) | (U(1) << 20) | (U(1) << 11))
#define SCTLR_AARCH32_EL1_RES1 \
            ((U(1) << 23) | (U(1) << 22) | (U(1) << 11) | \
             (U(1) << 4) | (U(1) << 3))

#define SCTLR_EL3_RES1    ((U(1) << 29) | (U(1) << 28) | (U(1) << 23) | \
            (U(1) << 22) | (U(1) << 18) | (U(1) << 16) | \
            (U(1) << 11) | (U(1) << 5) | (U(1) << 4))

//#define SCTLR_M_BIT        (ULL(1) << 0)
#define SCTLR_A_BIT        (ULL(1) << 1)
#define SCTLR_C_BIT        (ULL(1) << 2)
#define SCTLR_SA_BIT        (ULL(1) << 3)
#define SCTLR_SA0_BIT        (ULL(1) << 4)
#define SCTLR_CP15BEN_BIT    (ULL(1) << 5)
#define SCTLR_ITD_BIT        (ULL(1) << 7)
#define SCTLR_SED_BIT        (ULL(1) << 8)
#define SCTLR_UMA_BIT        (ULL(1) << 9)
//#define SCTLR_I_BIT        (ULL(1) << 12)
#define SCTLR_EnDB_BIT        (ULL(1) << 13)
#define SCTLR_DZE_BIT        (ULL(1) << 14)
#define SCTLR_UCT_BIT        (ULL(1) << 15)
#define SCTLR_NTWI_BIT        (ULL(1) << 16)
#define SCTLR_NTWE_BIT        (ULL(1) << 18)
#define SCTLR_WXN_BIT        (ULL(1) << 19)
#define SCTLR_UWXN_BIT        (ULL(1) << 20)
#define SCTLR_IESB_BIT        (ULL(1) << 21)
#define SCTLR_SPAN_BIT        (ULL(1) << 23)
#define SCTLR_E0E_BIT        (ULL(1) << 24)
#define SCTLR_EE_BIT        (ULL(1) << 25)
#define SCTLR_UCI_BIT        (ULL(1) << 26)
#define SCTLR_EnDA_BIT        (ULL(1) << 27)
#define SCTLR_EnIB_BIT        (ULL(1) << 30)
#define SCTLR_EnIA_BIT        (ULL(1) << 31)
#define SCTLR_DSSBS_BIT        (ULL(1) << 44)
#define SCTLR_RESET_VAL        SCTLR_EL3_RES1

/* HSTR_EL2 definitions */
#define HSTR_EL2_RESET_VAL    U(0x0)
#define HSTR_EL2_T_MASK        U(0xff)

/* VTTBR_EL2 definitions */
#define VTTBR_RESET_VAL        ULL(0x0)
#define VTTBR_VMID_MASK        ULL(0xff)
#define VTTBR_VMID_SHIFT    U(48)
#define VTTBR_BADDR_MASK    ULL(0xffffffffffff)
#define VTTBR_BADDR_SHIFT    U(0)

/* HCR definitions */
#define HCR_AMVOFFEN_BIT    (ULL(1) << 51)
#define HCR_API_BIT        (ULL(1) << 41)
#define HCR_APK_BIT        (ULL(1) << 40)
#define HCR_E2H_BIT        (ULL(1) << 34)
#define HCR_TGE_BIT        (ULL(1) << 27)
#define HCR_RW_SHIFT        U(31)
#define HCR_RW_BIT        (ULL(1) << HCR_RW_SHIFT)
#define HCR_AMO_BIT        (ULL(1) << 5)
#define HCR_IMO_BIT        (ULL(1) << 4)
#define HCR_FMO_BIT        (ULL(1) << 3)

/* ISR definitions */
#define ISR_A_SHIFT        U(8)
#define ISR_I_SHIFT        U(7)
#define ISR_F_SHIFT        U(6)

/* CPTR_EL3 definitions */
#define TCPAC_BIT        (U(1) << 31)
#define TAM_BIT            (U(1) << 30)
#define TTA_BIT            (U(1) << 20)
#define ESM_BIT            (U(1) << 12)
#define TFP_BIT            (U(1) << 10)
#define CPTR_EZ_BIT        (U(1) << 8)
#define CPTR_EL3_RESET_VAL    U(0x0)

/* CPTR_EL2 definitions */
#define CPTR_EL2_RES1        ((ULL(3) << 12) | (ULL(1) << 9) | (ULL(0xff)))
#define CPTR_EL2_TCPAC_BIT    (ULL(1) << 31)
#define CPTR_EL2_TAM_BIT    (ULL(1) << 30)
#define CPTR_EL2_SMEN_MASK    ULL(0x3)
#define CPTR_EL2_SMEN_SHIFT    U(24)
#define CPTR_EL2_TTA_BIT    (ULL(1) << 20)
#define CPTR_EL2_TSM_BIT    (ULL(1) << 12)
#define CPTR_EL2_TFP_BIT    (ULL(1) << 10)
#define CPTR_EL2_TZ_BIT        (ULL(1) << 8)
#define CPTR_EL2_RESET_VAL    CPTR_EL2_RES1

/* CPSR/SPSR definitions */
#define DAIF_FIQ_BIT        (U(1) << 0)
#define DAIF_IRQ_BIT        (U(1) << 1)
#define DAIF_ABT_BIT        (U(1) << 2)
#define DAIF_DBG_BIT        (U(1) << 3)
#define SPSR_DAIF_SHIFT        U(6)
#define SPSR_DAIF_MASK        U(0xf)

#define SPSR_AIF_SHIFT        U(6)
#define SPSR_AIF_MASK        U(0x7)

#define SPSR_E_SHIFT        U(9)
#define SPSR_E_MASK        U(0x1)
#define SPSR_E_LITTLE        U(0x0)
#define SPSR_E_BIG        U(0x1)

#define SPSR_T_SHIFT        U(5)
#define SPSR_T_MASK        U(0x1)
#define SPSR_T_ARM        U(0x0)
#define SPSR_T_THUMB        U(0x1)

#define SPSR_M_SHIFT        U(4)
#define SPSR_M_MASK        U(0x1)
#define SPSR_M_AARCH64        U(0x0)
#define SPSR_M_AARCH32        U(0x1)

#define DISABLE_ALL_EXCEPTIONS \
        (DAIF_FIQ_BIT | DAIF_IRQ_BIT | DAIF_ABT_BIT | DAIF_DBG_BIT)

#define DISABLE_INTERRUPTS    (DAIF_FIQ_BIT | DAIF_IRQ_BIT)

/*
 * HI-VECTOR address for AArch32 state
 */
#define HI_VECTOR_BASE        U(0xFFFF0000)

/*
 * TCR definitions
 */
#define TCR_EL3_RES1        ((ULL(1) << 31) | (ULL(1) << 23))
#define TCR_EL2_RES1        ((ULL(1) << 31) | (ULL(1) << 23))
#define TCR_EL1_IPS_SHIFT    U(32)
#define TCR_EL2_PS_SHIFT    U(16)
#define TCR_EL3_PS_SHIFT    U(16)

#define TCR_TxSZ_MIN        ULL(16)
#define TCR_TxSZ_MIN_LPA2    UL(12)
#define TCR_TxSZ_MAX        ULL(39)
#define TCR_TxSZ_MAX_TTST    ULL(48)

#define TCR_T0SZ_SHIFT        U(0)
#define TCR_T1SZ_SHIFT        U(16)

/* (internal) physical address size bits in EL3/EL1 */
#define TCR_PS_BITS_4GB        ULL(0x0)
#define TCR_PS_BITS_64GB    ULL(0x1)
#define TCR_PS_BITS_1TB        ULL(0x2)
#define TCR_PS_BITS_4TB        ULL(0x3)
#define TCR_PS_BITS_16TB    ULL(0x4)
#define TCR_PS_BITS_256TB    ULL(0x5)

#define ADDR_MASK_48_TO_63    ULL(0xFFFF000000000000)
#define ADDR_MASK_44_TO_47    ULL(0x0000F00000000000)
#define ADDR_MASK_42_TO_43    ULL(0x00000C0000000000)
#define ADDR_MASK_40_TO_41    ULL(0x0000030000000000)
#define ADDR_MASK_36_TO_39    ULL(0x000000F000000000)
#define ADDR_MASK_32_TO_35    ULL(0x0000000F00000000)

#define TCR_RGN_INNER_NC    (ULL(0x0) << 8)
#define TCR_RGN_INNER_WBA    (ULL(0x1) << 8)
#define TCR_RGN_INNER_WT    (ULL(0x2) << 8)
#define TCR_RGN_INNER_WBNA    (ULL(0x3) << 8)

#define TCR_RGN_OUTER_NC    (ULL(0x0) << 10)
#define TCR_RGN_OUTER_WBA    (ULL(0x1) << 10)
#define TCR_RGN_OUTER_WT    (ULL(0x2) << 10)
#define TCR_RGN_OUTER_WBNA    (ULL(0x3) << 10)

#define TCR_SH_NON_SHAREABLE    (ULL(0x0) << 12)
#define TCR_SH_OUTER_SHAREABLE    (ULL(0x2) << 12)
#define TCR_SH_INNER_SHAREABLE    (ULL(0x3) << 12)

#define TCR_RGN1_INNER_NC    (ULL(0x0) << 24)
#define TCR_RGN1_INNER_WBA    (ULL(0x1) << 24)
#define TCR_RGN1_INNER_WT    (ULL(0x2) << 24)
#define TCR_RGN1_INNER_WBNA    (ULL(0x3) << 24)

#define TCR_RGN1_OUTER_NC    (ULL(0x0) << 26)
#define TCR_RGN1_OUTER_WBA    (ULL(0x1) << 26)
#define TCR_RGN1_OUTER_WT    (ULL(0x2) << 26)
#define TCR_RGN1_OUTER_WBNA    (ULL(0x3) << 26)

#define TCR_SH1_NON_SHAREABLE    (ULL(0x0) << 28)
#define TCR_SH1_OUTER_SHAREABLE    (ULL(0x2) << 28)
#define TCR_SH1_INNER_SHAREABLE    (ULL(0x3) << 28)

#define TCR_TG0_SHIFT        U(14)
#define TCR_TG0_MASK        ULL(3)
#define TCR_TG0_4K        (ULL(0) << TCR_TG0_SHIFT)
#define TCR_TG0_64K        (ULL(1) << TCR_TG0_SHIFT)
#define TCR_TG0_16K        (ULL(2) << TCR_TG0_SHIFT)

#define TCR_TG1_SHIFT        U(30)
#define TCR_TG1_MASK        ULL(3)
#define TCR_TG1_16K        (ULL(1) << TCR_TG1_SHIFT)
#define TCR_TG1_4K        (ULL(2) << TCR_TG1_SHIFT)
#define TCR_TG1_64K        (ULL(3) << TCR_TG1_SHIFT)

#define TCR_EPD0_BIT        (ULL(1) << 7)
#define TCR_EPD1_BIT        (ULL(1) << 23)

#define MODE_SP_SHIFT        U(0x0)
#define MODE_SP_MASK        U(0x1)
#define MODE_SP_EL0        U(0x0)
#define MODE_SP_ELX        U(0x1)

#define MODE_RW_SHIFT        U(0x4)
#define MODE_RW_MASK        U(0x1)
#define MODE_RW_64        U(0x0)
#define MODE_RW_32        U(0x1)

#define MODE_EL_SHIFT        U(0x2)
#define MODE_EL_MASK        U(0x3)
#define MODE_EL3        U(0x3)
#define MODE_EL2        U(0x2)
#define MODE_EL1        U(0x1)
#define MODE_EL0        U(0x0)

#define MODE32_SHIFT        U(0)
#define MODE32_MASK        U(0xf)
#define MODE32_usr        U(0x0)
#define MODE32_fiq        U(0x1)
#define MODE32_irq        U(0x2)
#define MODE32_svc        U(0x3)
#define MODE32_mon        U(0x6)
#define MODE32_abt        U(0x7)
#define MODE32_hyp        U(0xa)
#define MODE32_und        U(0xb)
#define MODE32_sys        U(0xf)

#define GET_RW(mode)        (((mode) >> MODE_RW_SHIFT) & MODE_RW_MASK)
#define GET_EL(mode)        (((mode) >> MODE_EL_SHIFT) & MODE_EL_MASK)
#define GET_SP(mode)        (((mode) >> MODE_SP_SHIFT) & MODE_SP_MASK)
#define GET_M32(mode)        (((mode) >> MODE32_SHIFT) & MODE32_MASK)

#define SPSR_64(el, sp, daif)                \
    ((MODE_RW_64 << MODE_RW_SHIFT) |        \
    (((el) & MODE_EL_MASK) << MODE_EL_SHIFT) |    \
    (((sp) & MODE_SP_MASK) << MODE_SP_SHIFT) |    \
    (((daif) & SPSR_DAIF_MASK) << SPSR_DAIF_SHIFT))

#define SPSR_MODE32(mode, isa, endian, aif)        \
    ((MODE_RW_32 << MODE_RW_SHIFT) |        \
    (((mode) & MODE32_MASK) << MODE32_SHIFT) |    \
    (((isa) & SPSR_T_MASK) << SPSR_T_SHIFT) |    \
    (((endian) & SPSR_E_MASK) << SPSR_E_SHIFT) |    \
    (((aif) & SPSR_AIF_MASK) << SPSR_AIF_SHIFT))

/*
 * TTBR Definitions
 */
#define TTBR_CNP_BIT        ULL(0x1)

/*
 * CTR_EL0 definitions
 */
#define CTR_CWG_SHIFT        U(24)
#define CTR_CWG_MASK        U(0xf)
#define CTR_ERG_SHIFT        U(20)
#define CTR_ERG_MASK        U(0xf)
#define CTR_DMINLINE_SHIFT    U(16)
#define CTR_DMINLINE_MASK    U(0xf)
#define CTR_L1IP_SHIFT        U(14)
#define CTR_L1IP_MASK        U(0x3)
#define CTR_IMINLINE_SHIFT    U(0)
#define CTR_IMINLINE_MASK    U(0xf)

#define MAX_CACHE_LINE_SIZE    U(0x800) /* 2KB */

/* Exception Syndrome register bits and bobs */
#define ESR_EC_SHIFT            U(26)
#define ESR_EC_MASK            U(0x3f)
#define ESR_EC_LENGTH            U(6)
#define EC_UNKNOWN            U(0x0)
#define EC_WFE_WFI            U(0x1)
#define EC_AARCH32_CP15_MRC_MCR        U(0x3)
#define EC_AARCH32_CP15_MRRC_MCRR    U(0x4)
#define EC_AARCH32_CP14_MRC_MCR        U(0x5)
#define EC_AARCH32_CP14_LDC_STC        U(0x6)
#define EC_FP_SIMD            U(0x7)
#define EC_AARCH32_CP10_MRC        U(0x8)
#define EC_AARCH32_CP14_MRRC_MCRR    U(0xc)
#define EC_ILLEGAL            U(0xe)
#define EC_AARCH32_SVC            U(0x11)
#define EC_AARCH32_HVC            U(0x12)
#define EC_AARCH32_SMC            U(0x13)
#define EC_AARCH64_SVC            U(0x15)
#define EC_AARCH64_HVC            U(0x16)
#define EC_AARCH64_SMC            U(0x17)
#define EC_AARCH64_SYS            U(0x18)
#define EC_IABORT_LOWER_EL        U(0x20)
#define EC_IABORT_CUR_EL        U(0x21)
#define EC_PC_ALIGN            U(0x22)
#define EC_DABORT_LOWER_EL        U(0x24)
#define EC_DABORT_CUR_EL        U(0x25)
#define EC_SP_ALIGN            U(0x26)
#define EC_AARCH32_FP            U(0x28)
#define EC_AARCH64_FP            U(0x2c)
#define EC_SERROR            U(0x2f)

/*
 * External Abort bit in Instruction and Data Aborts synchronous exception
 * syndromes.
 */
#define ESR_ISS_EABORT_EA_BIT        U(9)

#define EC_BITS(x)            (((x) >> ESR_EC_SHIFT) & ESR_EC_MASK)

/* Reset bit inside the Reset management register for EL3 (RMR_EL3) */
#define RMR_RESET_REQUEST_SHIFT     U(0x1)
#define RMR_WARM_RESET_CPU        (U(1) << RMR_RESET_REQUEST_SHIFT)

/*******************************************************************************
 * Definitions of register offsets, fields and macros for CPU system
 * instructions.
 ******************************************************************************/

#define TLBI_ADDR_SHIFT        U(12)
#define TLBI_ADDR_MASK        ULL(0x00000FFFFFFFFFFF)
#define TLBI_ADDR(x)        (((x) >> TLBI_ADDR_SHIFT) & TLBI_ADDR_MASK)


/*******************************************************************************
 * Definitions for system register interface to SVE
 ******************************************************************************/
#define ZCR_EL3            S3_6_C1_C2_0
#define ZCR_EL2            S3_4_C1_C2_0

/* ZCR_EL3 definitions */
#define ZCR_EL3_LEN_MASK    U(0xf)

/* ZCR_EL2 definitions */
#define ZCR_EL2_LEN_MASK    U(0xf)

/*******************************************************************************
 * Definitions for system register interface to SME
 ******************************************************************************/
#define ID_AA64SMFR0_EL1        S3_0_C0_C4_5
#define SVCR                S3_3_C4_C2_2
#define TPIDR2_EL0            S3_3_C13_C0_5
#define SMCR_EL2            S3_4_C1_C2_6

/* ID_AA64SMFR0_EL1 definitions */
#define ID_AA64SMFR0_EL1_FA64_BIT    (UL(1) << 63)

/* SVCR definitions */
#define SVCR_ZA_BIT            (U(1) << 1)
#define SVCR_SM_BIT            (U(1) << 0)

/* SMPRI_EL1 definitions */
#define SMPRI_EL1_PRIORITY_SHIFT    U(0)
#define SMPRI_EL1_PRIORITY_MASK        U(0xf)

/* SMPRIMAP_EL2 definitions */
/* Register is composed of 16 priority map fields of 4 bits numbered 0-15. */
#define SMPRIMAP_EL2_MAP_SHIFT(pri)    U((pri) * 4)
#define SMPRIMAP_EL2_MAP_MASK        U(0xf)

/* SMCR_ELx definitions */
#define SMCR_ELX_LEN_SHIFT        U(0)
#define SMCR_ELX_LEN_MASK        U(0x1ff)
#define SMCR_ELX_FA64_BIT        (U(1) << 31)

/*******************************************************************************
 * Definitions of MAIR encodings for device and normal memory
 ******************************************************************************/
/*
 * MAIR encodings for device memory attributes.
 */
#define MAIR_DEV_nGnRnE        ULL(0x0)
#define MAIR_DEV_nGnRE        ULL(0x4)
#define MAIR_DEV_nGRE        ULL(0x8)
#define MAIR_DEV_GRE        ULL(0xc)

/*
 * MAIR encodings for normal memory attributes.
 *
 * Cache Policy
 *  WT:     Write Through
 *  WB:     Write Back
 *  NC:     Non-Cacheable
 *
 * Transient Hint
 *  NTR: Non-Transient
 *  TR:     Transient
 *
 * Allocation Policy
 *  RA:     Read Allocate
 *  WA:     Write Allocate
 *  RWA: Read and Write Allocate
 *  NA:     No Allocation
 */
#define MAIR_NORM_WT_TR_WA    ULL(0x1)
#define MAIR_NORM_WT_TR_RA    ULL(0x2)
#define MAIR_NORM_WT_TR_RWA    ULL(0x3)
#define MAIR_NORM_NC        ULL(0x4)
#define MAIR_NORM_WB_TR_WA    ULL(0x5)
#define MAIR_NORM_WB_TR_RA    ULL(0x6)
#define MAIR_NORM_WB_TR_RWA    ULL(0x7)
#define MAIR_NORM_WT_NTR_NA    ULL(0x8)
#define MAIR_NORM_WT_NTR_WA    ULL(0x9)
#define MAIR_NORM_WT_NTR_RA    ULL(0xa)
#define MAIR_NORM_WT_NTR_RWA    ULL(0xb)
#define MAIR_NORM_WB_NTR_NA    ULL(0xc)
#define MAIR_NORM_WB_NTR_WA    ULL(0xd)
#define MAIR_NORM_WB_NTR_RA    ULL(0xe)
#define MAIR_NORM_WB_NTR_RWA    ULL(0xf)

#define MAIR_NORM_OUTER_SHIFT    U(4)

#define MAKE_MAIR_NORMAL_MEMORY(inner, outer)    \
        ((inner) | ((outer) << MAIR_NORM_OUTER_SHIFT))

/* PAR_EL1 fields */
#define PAR_F_SHIFT    U(0)
#define PAR_F_MASK    ULL(0x1)
#define PAR_ADDR_SHIFT    U(12)
#define PAR_ADDR_MASK    (BIT(40) - ULL(1)) /* 40-bits-wide page address */

/*******************************************************************************
 * Armv8.1 Registers - Privileged Access Never Registers
 ******************************************************************************/
#define PAN            S3_0_C4_C2_3
#define PAN_BIT            BIT(22)

/*******************************************************************************
 * Armv8.3 Pointer Authentication Registers
 ******************************************************************************/
#define APIAKeyLo_EL1        S3_0_C2_C1_0
#define APIAKeyHi_EL1        S3_0_C2_C1_1
#define APIBKeyLo_EL1        S3_0_C2_C1_2
#define APIBKeyHi_EL1        S3_0_C2_C1_3
#define APDAKeyLo_EL1        S3_0_C2_C2_0
#define APDAKeyHi_EL1        S3_0_C2_C2_1
#define APDBKeyLo_EL1        S3_0_C2_C2_2
#define APDBKeyHi_EL1        S3_0_C2_C2_3
#define APGAKeyLo_EL1        S3_0_C2_C3_0
#define APGAKeyHi_EL1        S3_0_C2_C3_1

/*******************************************************************************
 * Armv8.4 Data Independent Timing Registers
 ******************************************************************************/
#define DIT            S3_3_C4_C2_5
#define DIT_BIT            BIT(24)

/*******************************************************************************
 * Armv8.5 - new MSR encoding to directly access PSTATE.SSBS field
 ******************************************************************************/
#define SSBS            S3_3_C4_C2_6

/*******************************************************************************
 * Armv8.5 - Memory Tagging Extension Registers
 ******************************************************************************/
#define TFSRE0_EL1        S3_0_C5_C6_1
#define TFSR_EL1        S3_0_C5_C6_0
#define RGSR_EL1        S3_0_C1_C0_5
#define GCR_EL1            S3_0_C1_C0_6

/*******************************************************************************
 * Armv8.6 - Fine Grained Virtualization Traps Registers
 ******************************************************************************/
#define HFGRTR_EL2        S3_4_C1_C1_4
#define HFGWTR_EL2        S3_4_C1_C1_5
#define HFGITR_EL2        S3_4_C1_C1_6
#define HDFGRTR_EL2        S3_4_C3_C1_4
#define HDFGWTR_EL2        S3_4_C3_C1_5

/*******************************************************************************
 * Armv8.6 - Enhanced Counter Virtualization Registers
 ******************************************************************************/
#define CNTPOFF_EL2  S3_4_C14_C0_6

/*******************************************************************************
 * FEAT_HCX - Extended Hypervisor Configuration Register
 ******************************************************************************/
#define HCRX_EL2        S3_4_C1_C2_2
#define HCRX_EL2_FGTnXS_BIT    (UL(1) << 4)
#define HCRX_EL2_FnXS_BIT    (UL(1) << 3)
#define HCRX_EL2_EnASR_BIT    (UL(1) << 2)
#define HCRX_EL2_EnALS_BIT    (UL(1) << 1)
#define HCRX_EL2_EnAS0_BIT    (UL(1) << 0)

/* Added for XLAT */

#define ID_AA64MMFR2_EL1_ST_WIDTH    UL(4)
#define ID_AA64MMFR2_EL1_CNP_WIDTH    UL(4)

/* ID_AA64PFR0_EL1 definitions */
#define ID_AA64PFR0_EL1_SVE_SHIFT    UL(32)
#define ID_AA64PFR0_EL1_SVE_WIDTH    UL(4)

/* RNDR definitions */
#define ID_AA64ISAR0_EL1_RNDR_SHIFT        UL(60)
#define ID_AA64ISAR0_EL1_RNDR_WIDTH        UL(4)

/* ID_AA64MMFR1_EL1 definitions */
#define ID_AA64MMFR1_EL1_VMIDBits_SHIFT        UL(4)
#define ID_AA64MMFR1_EL1_VMIDBits_WIDTH        UL(4)
#define ID_AA64MMFR1_EL1_VMIDBits_8        UL(0)
#define ID_AA64MMFR1_EL1_VMIDBits_16        UL(2)

#define ID_AA64MMFR0_EL1_TGRAN4_WIDTH        UL(4)
#define ID_AA64MMFR0_EL1_TGRAN16_WIDTH        UL(4)
#define ID_AA64MMFR0_EL1_TGRAN64_WIDTH        UL(4)
#define ID_AA64MMFR0_EL1_TGRAN4_2_SHIFT        U(40)
#define ID_AA64MMFR0_EL1_TGRAN4_2_WIDTH        U(4)
#define ID_AA64MMFR0_EL1_TGRAN4_LPA2        UL(0x1)
#define ID_AA64MMFR0_EL1_TGRAN4_2_LPA2        UL(0x3)
#define ID_AA64MMFR0_EL1_TGRAN4_2_TGRAN4    UL(0x0)

#define ID_AA64DFR0_EL1_PMUVer_SHIFT        UL(8)
#define ID_AA64DFR0_EL1_PMUVer_WIDTH        UL(4)

#define TCR_PS_BITS_4PB        INPLACE(TCR_EL2_IPS, UL(6))
#define SCTLR_ELx_M_BIT            (UL(1) << 0)
#define MAIR_DEV_NGNRE        UL(0x4)

#define TCR_EL2_T0SZ_SHIFT    UL(0)
#define TCR_EL2_T0SZ_WIDTH    UL(6)

#define TCR_EL2_T1SZ_SHIFT    UL(16)
#define TCR_EL2_T1SZ_WIDTH    UL(6)

#define TCR_EL2_EPD0_BIT    (UL(1) << 7)

#define TCR_EL2_IRGN0_SHIFT    UL(8)
#define TCR_EL2_IRGN0_WIDTH    UL(2)
#define TCR_EL2_IRGN0_WBWA    INPLACE(TCR_EL2_IRGN0, UL(1))

#define TCR_EL2_ORGN0_SHIFT    UL(10)
#define TCR_EL2_ORGN0_WIDTH    UL(2)
#define TCR_EL2_ORGN0_WBWA    INPLACE(TCR_EL2_ORGN0, UL(1))

#define TCR_EL2_IRGN1_SHIFT    UL(24)
#define TCR_EL2_IRGN1_WIDTH    UL(2)
#define TCR_EL2_IRGN1_WBWA    INPLACE(TCR_EL2_IRGN1, UL(1))

#define TCR_EL2_ORGN1_SHIFT    UL(26)
#define TCR_EL2_ORGN1_WIDTH    UL(2)
#define TCR_EL2_ORGN1_WBWA    INPLACE(TCR_EL2_ORGN1, UL(1))

#define TCR_EL2_SH0_SHIFT    UL(12)
#define TCR_EL2_SH0_WIDTH    UL(2)
#define TCR_EL2_SH0_IS        INPLACE(TCR_EL2_SH0, UL(3))

#define TCR_EL2_SH1_SHIFT    UL(28)
#define TCR_EL2_SH1_WIDTH    UL(2)
#define TCR_EL2_SH1_IS        INPLACE(TCR_EL2_SH1, UL(3))

#define TCR_EL2_TG0_SHIFT    UL(14)
#define TCR_EL2_TG0_WIDTH    UL(2)
#define TCR_EL2_TG0_4K        INPLACE(TCR_EL2_TG0, UL(0))

#define TCR_EL2_TG1_SHIFT    UL(30)
#define TCR_EL2_TG1_WIDTH    UL(2)
#define TCR_EL2_TG1_4K        INPLACE(TCR_EL2_TG1, UL(2))

#define TCR_EL2_IPS_SHIFT    UL(32)
#define TCR_EL2_IPS_WIDTH    UL(3)

#define TCR_EL2_DS_SHIFT    UL(59)
#define TCR_EL2_DS_WIDTH    UL(1)
#define TCR_EL2_DS_LPA2_EN    INPLACE(TCR_EL2_DS, UL(1))

#define TCR_EL2_AS        (UL(1) << 36)
#define TCR_EL2_HPD0        (UL(1) << 41)
#define TCR_EL2_HPD1        (UL(1) << 42)

#define TTBRx_EL2_BADDR_SHIFT    1
#define TTBRx_EL2_BADDR_WIDTH    47

#define PARANGE_0000_WIDTH    U(32)
#define PARANGE_0001_WIDTH    U(36)
#define PARANGE_0010_WIDTH    U(40)
#define PARANGE_0011_WIDTH    U(42)
#define PARANGE_0100_WIDTH    U(44)
#define PARANGE_0101_WIDTH    U(48)
#define PARANGE_0110_WIDTH    U(52)

#define ID_AA64MMFR0_EL1_PARANGE_WIDTH    UL(4)

#endif /* _ARCH_H_ */
