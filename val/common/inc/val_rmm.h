/*
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_RMM_H_
#define _VAL_RMM_H_

/* ACS has 1:1 mapping for REC_NUM and REC_MPIDR */
#define REC_NUM(n) n

/* RMI SMC FID */
#define RMI_VERSION                  0xC4000150
#define RMI_GRANULE_DELEGATE         0xC4000151
#define RMI_GRANULE_UNDELEGATE       0xC4000152
#define RMI_DATA_CREATE              0xC4000153
#define RMI_DATA_CREATE_UNKNOWN      0xC4000154
#define RMI_DATA_DESTROY             0xC4000155
#define RMI_REALM_ACTIVATE           0xC4000157
#define RMI_REALM_CREATE             0xC4000158
#define RMI_REALM_DESTROY            0xC4000159
#define RMI_REC_CREATE               0xC400015A
#define RMI_REC_DESTROY              0xC400015B
#define RMI_REC_ENTER                0xC400015C
#define RMI_RTT_CREATE               0xC400015D
#define RMI_RTT_DESTROY              0xC400015E
#define RMI_RTT_MAP_PROTECTED        0xC4000160
#define RMI_RTT_MAP_UNPROTECTED      0xC400015F
#define RMI_RTT_READ_ENTRY           0xC4000161
#define RMI_RTT_UNMAP_UNPROTECTED    0xC4000162
#define RMI_PSCI_COMPLETE            0xC4000164
#define RMI_FEATURES                 0xC4000165
#define RMI_RTT_FOLD                 0xC4000166
#define RMI_REC_AUX_COUNT            0xC4000167
#define RMI_RTT_INIT_RIPAS           0xC4000168
#define RMI_RTT_SET_RIPAS            0xC4000169
#define RMI_GRANULE_IO_DELEGATE      0xC4000170
#define RMI_GRANULE_IO_UNDELEGATE    0xC4000171
#define RMI_IO_CREATE                0xC4000172
#define RMI_IO_DESTROY               0xC4000173
#define RMI_PDEV_ABORT               0xC4000174
#define RMI_PDEV_COMMUNICATE         0xC4000175
#define RMI_PDEV_CREATE              0xC4000176
#define RMI_PDEV_DESTROY             0xC4000177
#define RMI_PDEV_GET_STATE           0xC4000178
#define RMI_PDEV_IDE_RESET           0xC4000179
#define RMI_PDEV_NOTIFY              0xC400017A
#define RMI_PDEV_SET_KEY             0xC400017B
#define RMI_PDEV_STOP                0xC400017C
#define RMI_VDEV_ABORT               0xC4000185
#define RMI_VDEV_COMMUNICATE         0xC4000186
#define RMI_VDEV_CREATE              0xC4000187
#define RMI_VDEV_DESTROY             0xC4000188
#define RMI_VDEV_GET_STATE           0xC4000189
#define RMI_VDEV_STOP                0xC400018A

/* v1.1 Planes SMC IDs */
#define RMI_RTT_SET_S2AP               0xC400018B
#define RMI_RTT_AUX_CREATE             0xC400017D
#define RMI_RTT_AUX_DESTROY            0xC400017E
#define RMI_RTT_AUX_FOLD               0xC400017F
#define RMI_RTT_AUX_MAP_PROTECTED      0xC4000180
#define RMI_RTT_AUX_MAP_UNPROTECTED    0xC4000181
#define RMI_RTT_AUX_UNMAP_PROTECTED    0xC4000183
#define RMI_RTT_AUX_UNMAP_UNPROTECTED  0xC4000184

/* RmiFeature type */
#define RMI_FEATURE_FALSE     0
#define RMI_FEATURE_TRUE      1

#define RMI_FEATURE_REGISTER_0_INDEX 0

/* RmiPlaneRttFeature type*/
#define RMI_PLANE_RTT_AUX              0
#define RMI_PLANE_RTT_AUX_SINGLE       1
#define RMI_PLANE_RTT_SINGLE           2

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
#define RMI_EXIT_IO            7
#define RMI_EXIT_RTT_REQUES    8
#define RMI_EXIT_S2AP_CHANGE   9

#define RMI_VALIDN_NS 1

/* RmiRecRunnable types */
#define RMI_NOT_RUNNABLE 0
#define RMI_RUNNABLE     1

/* RmiRipas types */
#define RMI_EMPTY        0
#define RMI_RAM          1
#define RMI_DESTROYED    2
#define RMI_IO           3

/* RmiRttEntryState types */
#define RMI_UNASSIGNED          0
#define RMI_ASSIGNED            1
#define RMI_TABLE               2
#define RMI_ASSIGNED_IO_PRIVATE 3
#define RMI_ASSIGNED_IO_SHARED  4
#define RMI_AUX_DESTROYED       5

/* RmiStatusCode types */
#define RMI_SUCCESS                0
#define RMI_ERROR_INPUT            1
#define RMI_ERROR_REALM            2
#define RMI_ERROR_REC              3
#define RMI_ERROR_RTT              4
#define RMI_ERROR_DEVICE           5
#define RMI_ERROR_NOT_SUPPORTED    6
#define RMI_ERROR_RTT_AUX          7

/* RmiDataMeasureContent types */
#define RMI_NO_MEASURE_CONTENT      0
#define RMI_MEASURE_CONTENT         1

/* RmiInjectSea types */
#define RMI_NO_INJECT_SEA           0
#define RMI_INJECT_SEA              1

/* RmiTrap types */
#define RMI_NO_TRAP                 0
#define RMI_TRAP                    1

/* RmiResponse types */
#define RMI_ACCEPT                  0
#define RMI_REJECT                  1

/* RmiUnprotectedS2AP type */
#define RMI_UNPROTECTED_S2AP_NO_ACCESS  0
#define RMI_UNPROTECTED_S2AP_RO         1
#define RMI_UNPROTECTED_S2AP_WO         2
#define RMI_UNPROTECTED_S2AP_RW         3

/* RmiBoolean type */
#define RMI_FALSE                   0
#define RMI_TRUE                    1

/* RmiIoAction type */
#define RMI_IO_GET_INTERFACE_REPORT     0
#define RMI_IO_GET_MEASUREMENTS         1
#define RMI_IO_LOCK                     2
#define RMI_IO_START                    3
#define RMI_IO_STOP                     4

/* RmiIoEnterStatus type */
#define RMI_IO_SUCCESS              0
#define RMI_IO_ERROR                1
#define RMI_IO_NONE                 2

/* RmiIoRequestType type */
#define RMI_DISCOVERY               0
#define RMI_CMA_SPDM                1
#define RMI_SECURE_CMA_SPDM         2

/* RmiIoShared type */
#define RMI_IO_PRIVATE              0
#define RMI_IO_SHARED               1

/* RmiPdevClass type */
#define RMI_PCIE                    0

/* RmiPdevEvent type */
#define RMI_IDE_KEY_REFRESH         0

/* RmiPdevState type */
#define RMI_PDEV_NEW                0
#define RMI_PDEV_NEEDS_KEY          1
#define RMI_PDEV_HAS_KEY            2
#define RMI_PDEV_READY              3
#define RMI_PDEV_COMMUNICATING      4
#define RMI_PDEV_STOPPING           5
#define RMI_PDEV_STOPPED            6
#define RMI_PDEV_ERROR              7

/* RmiSignatureAlgorithm type */
#define RMI_SIG_RSASSA_3072         0
#define RMI_SIG_ECDSA_P256          1
#define RMI_SIG_ECDSA_P384          2

/* RmiVdevState type */
#define RMI_VDEV_READY              0
#define RMI_VDEV_COMMUNICATING      1
#define RMI_VDEV_STOPPING           2
#define RMI_VDEV_STOPPED            3
#define RMI_VDEV_ERROR              4

/* RmiIoExit flags */
#define RMI_IO_EXIT_CACHE    (0x1UL << 0)
#define RMI_IO_EXIT_SEND    (0x1UL << 1)
#define RMI_IO_EXIT_WAIT    (0x1UL << 2)
#define RMI_IO_EXIT_MULTI    (0x1UL << 3)

/* Create the return code of an ABI command from status & index */
#define PACK_CODE(status, index) (index << 8 | status)

/* Get the status field from the return code */
#define RMI_STATUS(ret)       ((ret) & 0xFF)
/* Get the index field from the return code */
#define RMI_INDEX(ret)        (((ret) >> 8) & 0xFF)

/* RSI SMC FID */
#define RSI_VERSION                           (0xC4000190)
#ifdef RMM_V_1_1
#define RSI_REALM_CONFIG                      (0xC40001AD)
#elif defined(RMM_V_1_0)
#define RSI_REALM_CONFIG                      (0xC4000196)
#endif
#define RSI_MEASUREMENT_READ                  (0xC4000192)
#define RSI_MEASUREMENT_EXTEND                (0xC4000193)
#define RSI_IPA_STATE_GET                     (0xC4000198)
#define RSI_IPA_STATE_SET                     (0xC4000197)
#define RSI_HOST_CALL                         (0xC4000199)
#define RSI_ATTESTATION_TOKEN_INIT            (0xC4000194)
#define RSI_ATTESTATION_TOKEN_CONTINUE        (0xC4000195)
#define RSI_FEATURES                          (0xC4000191)
#define RSI_MEM_GET_PERM_VALUE                (0xC40001A0)
#define RSI_MEM_SET_PERM_INDEX                (0xC40001A1)
#define RSI_MEM_SET_PERM_VALUE                (0xC40001A2)
#define RSI_PLANE_ENTER                       (0xC40001A3)
#define RSI_PLANE_REG_READ                    (0xC40001AE)
#define RSI_PLANE_REG_WRITE                   (0xC40001AF)
#define RSI_RDEV_CONTINUE                     (0xC40001A4)
#define RSI_RDEV_GET_DIGESTS                  (0xC40001A5)
#define RSI_RDEV_GET_INTERFACE_REPORT         (0xC40001A6)
#define RSI_RDEV_GET_MEASUREMENTS             (0xC40001A7)
#define RSI_RDEV_GET_STATE                    (0xC40001A8)
#define RSI_RDEV_LOCK                         (0xC40001A9)
#define RSI_RDEV_START                        (0xC40001AA)
#define RSI_RDEV_STOP                         (0xC40001AB)
#define RSI_RDEV_VALIDATE_IO                  (0xC40001AC)

/* RsiInterfaceVersion type */
#define RSI_MAJOR_VERSION    0
#define RSI_MINOR_VERSION    0

/* RsiStatusCode types */
#define RSI_SUCCESS                0
#define RSI_ERROR_INPUT            1
#define RSI_ERROR_STATE            2
#define RSI_ERROR_INCOMPLETE       3
#define RSI_ERROR_DEVICE           4

/* RsiHashAlgorithm type */
#define RSI_HASH_SHA_256 0
#define RSI_HASH_SHA_512 1

/* RsiRipas type */
#define RSI_EMPTY        0
#define RSI_RAM          1
#define RSI_DESTROYED    2
#define RSI_IO           3

/* RsiRipasChangeDestroyed type */
#define RSI_NO_CHANGE_DESTROYED     0
#define RSI_CHANGE_DESTROYED        1

/* RsiRipasChangeDestroyed type */
#define RSI_NO_CHANGE_DESTROYED    0
#define RSI_CHANGE_DESTROYED       1

/* RsiResponse types */
#define RSI_ACCEPT                 0
#define RSI_REJECT                 1

/* RsiGicOwner type */
#define RSI_GIC_OWNER_0            0
#define RSI_GIC_OWNER_N            1

/* RsiPlaneExitReason types */
#define RSI_EXIT_SYNC              0

/* PSI HVC/SMC IDs
 * PSI (Plane services interface) is a IMPDEF interface between Pn and P0 */
#define PSI_REALM_CONFIG RSI_REALM_CONFIG
#define PSI_P0_CALL      RSI_HOST_CALL
#define PSI_PRINT_MSG    0x88FFFFFF
#define PSI_TEST_SMC     0x88FFFFFE
#define PSI_TEST_HVC     0x88FFFFFD

/* RsiBoolean type */
#define RSI_FALSE                  0
#define RSI_TRUE                   1

/* RsiDeviceState type */
#define RSI_RDEV_NEW                0
#define RSI_RDEV_NEW_BUSY           1
#define RSI_RDEV_LOCKED             2
#define RSI_RDEV_LOCKED_BUSY        3
#define RSI_RDEV_STARTED            4
#define RSI_RDEV_STARTED_BUSY       5
#define RSI_RDEV_STOPPING           6
#define RSI_RDEV_STOPPED            7
#define RSI_RDEV_ERROR              8

/* RsiFeature type */
#define RSI_FEATURE_FALSE           0
#define RSI_FEATURE_TRUE            1

/* RsiIoShared type */
#define RSI_IO_PRIVATE              0
#define RSI_IO_SHARED               1

/* RsiTrap type */
#define RSI_NO_TRAP                 0
#define RSI_TRAP                    1

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
