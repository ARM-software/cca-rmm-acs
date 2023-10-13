/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_RMM_H_
#define _VAL_RMM_H_

/* ACS has 1:1 mapping for REC_NUM and REC_MPIDR */
#define REC_NUM(n) n

/* RMI SMC FID */
#define RMI_DATA_CREATE              0xC4000153
#define RMI_DATA_CREATE_UNKNOWN      0xC4000154
#define RMI_DATA_DESTROY             0xC4000155
#define RMI_GRANULE_DELEGATE         0xC4000151
#define RMI_GRANULE_UNDELEGATE       0xC4000152
#define RMI_REALM_ACTIVATE           0xC4000157
#define RMI_REALM_CREATE             0xC4000158
#define RMI_REALM_DESTROY            0xC4000159
#define RMI_REC_CREATE               0xC400015a
#define RMI_REC_DESTROY              0xC400015b
#define RMI_REC_ENTER                0xC400015c
#define RMI_RTT_CREATE               0xC400015d
#define RMI_RTT_DESTROY              0xC400015e
#define RMI_RTT_MAP_PROTECTED        0xC4000160
#define RMI_RTT_MAP_UNPROTECTED      0xC400015f
#define RMI_RTT_READ_ENTRY           0xC4000161
#define RMI_RTT_UNMAP_UNPROTECTED    0xC4000162
#define RMI_PSCI_COMPLETE            0xC4000164
#define RMI_FEATURES                 0xC4000165
#define RMI_RTT_FOLD                 0xC4000166
#define RMI_REC_AUX_COUNT            0xC4000167
#define RMI_RTT_INIT_RIPAS           0xC4000168
#define RMI_RTT_SET_RIPAS            0xC4000169
#define RMI_VERSION                  0xC4000150

/* RmiFeature type */
#define RMI_FEATURE_FALSE  0
#define RMI_FEATURE_TRUE      1

#define RMI_FEATURE_REGISTER_0_INDEX 0

/* RmiInterfaceVersion type */
#define RMI_MAJOR_VERSION    0
#define RMI_MINOR_VERSION    0

#define RMI_VERSION_GET_MAJOR(ver) (((ver) >> 16) & 0xFFFF)
#define RMI_VERSION_GET_MINOR(ver) ((ver) & 0xFFFF)

/* RmiRealmMeasurementAlgorithm types */
#define RMI_SHA256  0

/* RmiHashAlgorithm type */
#define RMI_HASH_SHA_256    0
#define RMI_HASH_SHA_512    1

/* RmiRecEmulatedMmio types */
#define RMI_NOT_EMULATED_MMIO 0
#define RMI_EMULATED_MMIO     1

/* RmiPmuOverflowStatus types */
#define RMI_PMU_OVERFLOW_NOT_ACTIVE  0
#define RMI_PMU_OVERFLOW_ACTIVE      1

/* RmiRecExitReason types */
#define RMI_EXIT_SYNC          0
#define RMI_EXIT_IRQ           1
#define RMI_EXIT_FIQ           2
#define RMI_EXIT_PSCI          3
#define RMI_EXIT_RIPAS_CHANGE  4
#define RMI_EXIT_HOST_CALL     5
#define RMI_EXIT_SERROR        6

#define RMI_VALIDN_NS 1

/* RmiRecRunnable types */
#define RMI_NOT_RUNNABLE 0
#define RMI_RUNNABLE     1

/* RmiRipas types */
#define RMI_EMPTY        0
#define RMI_RAM          1
#define RMI_DESTROYED    2

/* RmiRttEntryState types */
#define RMI_UNASSIGNED        0
#define RMI_ASSIGNED          1
#define RMI_TABLE             2

/* RmiStatusCode types */
#define RMI_SUCCESS                0
#define RMI_ERROR_INPUT            1
#define RMI_ERROR_REALM            2
#define RMI_ERROR_REC              3
#define RMI_ERROR_RTT              4

/* RmiDataMeasureContent types */
#define RMI_NO_MEASURE_CONTENT      0
#define RMI_MEASURE_CONTENT         1

/* RmiInjectSea types */
#define RMI_NO_INJECT_SEA           0
#define RMI_INJECT_SEA              1

/* RmiTrap types */
#define RMI_NO_TRAP                 0
#define RMI_TRAP                    1

/* Create the return code of an ABI command from status & index */
#define PACK_CODE(status, index) (index << 8 | status)

/* Get the status field from the return code */
#define RMI_STATUS(ret)       ((ret) & 0xFF)
/* Get the index field from the return code */
#define RMI_INDEX(ret)        (((ret) >> 8) & 0xFF)

/* RSI SMC FID */
#define RSI_VERSION              (0xC4000190)
#define RSI_REALM_CONFIG         (0xC4000196)
#define RSI_MEASUREMENT_READ     (0xC4000192)
#define RSI_MEASUREMENT_EXTEND   (0xC4000193)
#define RSI_IPA_STATE_GET        (0xC4000198)
#define RSI_IPA_STATE_SET        (0xC4000197)
#define RSI_HOST_CALL            (0xC4000199)
#define RSI_ATTESTATION_TOKEN_INIT        (0xC4000194)
#define RSI_ATTESTATION_TOKEN_CONTINUE        (0xC4000195)

/* RsiInterfaceVersion type */
#define RSI_MAJOR_VERSION    0
#define RSI_MINOR_VERSION    0

/* RsiStatusCode types */
#define RSI_SUCCESS                0
#define RSI_ERROR_INPUT            1
#define RSI_ERROR_STATE            2
#define RSI_ERROR_INCOMPLETE       3

/* RsiHashAlgorithm type */
#define RSI_HASH_SHA_256 0
#define RSI_HASH_SHA_512 1

/* RsiRipas type */
#define RSI_EMPTY        0
#define RSI_RAM          1
#define RSI_DESTROYED    2

/* RsiRipasChangeDestroyed type */
#define RSI_NO_CHANGE_DESTROYED 0
#define RSI_CHANGE_DESTROYED 1

/* RsiRipasChangeDestroyed type */
#define RSI_NO_CHANGE_DESTROYED    0
#define RSI_CHANGE_DESTROYED       1

/*
 * Defines member of structure and reserves space
 * for the next member with specified offset.
 */
#define SET_MEMBER(member, start, end)  \
    union {             \
        member;         \
        unsigned char reserved##end[end - start]; \
    }

/* Data abort ESR fields */
#define ESR_ISS_SET_UER 0x0      /* SET[12:11] = b'00 Recoverable state (UER) */
#define ESR_ISS_EA 0x0           /* EA[9] = b'0 */
#define ESR_ISS_DFSC_TTF_L3 0x7  /* DFSC[5:0] = b'00111 Translation fault, level 3 */
#define ESR_ISS_DFSC_TTF_L2 0x6  /* DFSC[5:0] = b'000110 Translation fault, level 2 */
#define ESR_ISS_DFSC_PF_L3 0xF  /* DFSC[5:0] = b'001111 Permission fault, level 3 */
#define ESR_EC_LOWER_EL 0x24     /* EC[31:26] = b'100100 DA from lower EL */
#define ESR_IA_EC_LOWER_EL 0x20     /* EC[31:26] = b'100000 IA from lower EL */
#define ESR_ISV_VALID 0x1        /* ISV[24] = b'1 ISS[23:14] hold a valid instruction syndrome */
#define ESR_SAS_WORD 0x2         /* SAS[23:22] = b'10 Word */
#define ESR_FnV      0x0        /* FnV[10] = b'0 Valid only if the DFSC code is 0b010000 */
#define ESR_WnR_WRITE 0x1        /* WnR[6] = b'1 Abort caused by writing to a memory location */
#define ESR_WnR_READ 0x0        /* WnR[6] = b'0 Abort caused by an reading from a memory location */

/* Instruction abort ESR fields */
#define ESR_ISS_IFSC_TTF_L3 0x7  /* IFSC[5:0] = b'00111 Translation fault, level 3 */
#endif /* _VAL_RMM_H_ */
