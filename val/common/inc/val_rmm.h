/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_RMM_H_
#define _VAL_RMM_H_

/* RMI SMC FID */
#define RMI_VERSION                  0xC4000150
#define RMI_GRANULE_DELEGATE         0xC4000151
#define RMI_GRANULE_UNDELEGATE       0xC4000152
#define RMI_DATA_CREATE              0xC4000153
#define RMI_RTT_DATA_MAP_INIT        0xC4000153
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
#define RMI_RTT_MAP_UNPROTECTED      0xC400015F
#define RMI_RTT_MAP_PROTECTED        0xC4000160
#define RMI_RTT_READ_ENTRY           0xC4000161
#define RMI_RTT_UNMAP_UNPROTECTED    0xC4000162
#define RMI_PSCI_COMPLETE            0xC4000164
#define RMI_FEATURES                 0xC4000165
#define RMI_RTT_FOLD                 0xC4000166
#define RMI_REC_AUX_COUNT            0xC4000167
#define RMI_RTT_INIT_RIPAS           0xC4000168
#define RMI_RTT_SET_RIPAS            0xC4000169
#define RMI_RMM_CONFIG_SET           0xC400016E
#define RMI_GRANULE_TRACKING_GET     0xC40001E1
#define RMI_GRANULE_TRACKING_SET     0xC40001E3
#define RMI_RMM_CONFIG_GET           0xC40001EC
#define RMI_RMM_STATE_GET            0xC40001EE
#define RMI_GRANULE_RANGE_DELEGATE   0xC40001F1
#define RMI_GRANULE_RANGE_UNDELEGATE 0xC40001F2
#define RMI_GPT_L1_CREATE            0xC40001F3
#define RMI_GPT_L1_DESTROY           0xC40001F4
#define RMI_RTT_DATA_MAP             0xC40001F5
#define RMI_RTT_DATA_UNMAP           0xC40001F6
#define RMI_RTT_UNPROT_MAP           0xC40001FB
#define RMI_RTT_UNPROT_UNMAP         0xC40001FC
#define RMI_REALM_TERMINATE          0xC4000201
#define RMI_RMM_ACTIVATE             0xC4000202
#define RMI_OP_CONTINUE              0xC4000203
#define RMI_OP_MEM_DONATE            0xC4000208
#define RMI_OP_MEM_RECLAIM           0xC4000209
#define RMI_OP_CANCEL                0xC400020A

/* RMI DA SMC FID */
#define RMI_PDEV_AUX_COUNT                0xC4000156
#define RMI_VDEV_AUX_COUNT                0xC4000160
#define RMI_RTT_DEV_VALIDATE              0xC4000163
#define RMI_PDEV_ABORT                    0xC4000174
#define RMI_PDEV_COMMUNICATE              0xC4000175
#define RMI_PDEV_CREATE                   0xC4000176
#define RMI_PDEV_DESTROY                  0xC4000177
#define RMI_PDEV_GET_STATE                0xC4000178
#define RMI_PDEV_IDE_RESET                0xC4000179
#define RMI_PDEV_IDE_KEY_REFRESH          0xC400017A
#define RMI_PDEV_STREAM_KEY_REFRESH       0xC400017A
#define RMI_PDEV_SET_PUBKEY               0xC400017B
#define RMI_PDEV_STOP                     0xC400017C
#define RMI_VDEV_ABORT                    0xC4000185
#define RMI_VDEV_COMMUNICATE              0xC4000186
#define RMI_VDEV_CREATE                   0xC4000187
#define RMI_VDEV_DESTROY                  0xC4000188
#define RMI_VDEV_GET_STATE                0xC4000189
#define RMI_VDEV_UNLOCK                   0xC400018A
#define RMI_VDEV_GET_INTERFACE_REPORT     0xC40001D0
#define RMI_VDEV_GET_MEASUREMENTS         0xC40001D1
#define RMI_VDEV_LOCK                     0xC40001D2
#define RMI_VDEV_START                    0xC40001D3
#define RMI_RTT_DEV_MAP                   0xC40001F7
#define RMI_RTT_DEV_UNMAP                 0xC40001F8
#define RMI_PDEV_STREAM_CONNECT           0xC4000204
#define RMI_PDEV_STREAM_DISCONNECT        0xC4000205
#define RMI_PDEV_STREAM_COMPLETE          0xC4000206
#define RMI_PDEV_STREAM_KEY_PURGE         0xC4000207

/* RMI PSMMU SMC FID */
#define RMI_PSMMU_IRQ_NOTIFY              0xC400016F
#define RMI_PSMMU_ACTIVATE                0xC40001D7
#define RMI_PSMMU_DEACTIVATE              0xC40001D8
#define RMI_PSMMU_ST_L2_CREATE            0xC40001DB
#define RMI_PSMMU_ST_L2_DESTROY           0xC40001DC
#define RMI_DPT_L0_CREATE                 0xC40001DD
#define RMI_DPT_L0_DESTROY                0xC40001DE
#define RMI_DPT_L1_CREATE                 0xC40001DF
#define RMI_DPT_L1_DESTROY                0xC40001E0
#define RMI_PSMMU_EVENT_CONSUME           0xC40001F0
#define RMI_PSMMU_INFO                    0xC400020E

/* v1.1 Planes SMC IDs */
#define RMI_RTT_AUX_CREATE             0xC400017D
#define RMI_RTT_AUX_DESTROY            0xC400017E
#define RMI_RTT_AUX_FOLD               0xC400017F
#define RMI_RTT_AUX_MAP_PROTECTED      0xC4000180
#define RMI_RTT_AUX_MAP_UNPROTECTED    0xC4000181
#define RMI_RTT_AUX_UNMAP_PROTECTED    0xC4000183
#define RMI_RTT_AUX_UNMAP_UNPROTECTED  0xC4000184
#define RMI_RTT_SET_S2AP               0xC400018B

/* v1.1 MEC SMC IDs */
#define RMI_MEC_SET_SHARED             0xC400018C
#define RMI_MEC_SET_PRIVATE            0xC400018D

/* Create the return code of an ABI command from status & index */
#define PACK_CODE(status, index) (index << 8 | status)

/* RSI SMC FID */
#define RSI_VERSION                           (0xC4000190)
#define RSI_FEATURES                          (0xC4000191)
#define RSI_MEASUREMENT_READ                  (0xC4000192)
#define RSI_MEASUREMENT_EXTEND                (0xC4000193)
#define RSI_ATTESTATION_TOKEN_INIT            (0xC4000194)
#define RSI_ATTESTATION_TOKEN_CONTINUE        (0xC4000195)
#define RSI_REALM_CONFIG                      (0xC4000196)
#define RSI_IPA_STATE_SET                     (0xC4000197)
#define RSI_IPA_STATE_GET                     (0xC4000198)
#define RSI_HOST_CALL                         (0xC4000199)

/* RSI Planes SMC FID */
#define RSI_MEM_GET_PERM_VALUE                (0xC40001A0)
#define RSI_MEM_SET_PERM_INDEX                (0xC40001A1)
#define RSI_MEM_SET_PERM_VALUE                (0xC40001A2)
#define RSI_PLANE_ENTER                       (0xC40001A3)
#define RSI_PLANE_REG_READ                    (0xC40001AE)
#define RSI_PLANE_REG_WRITE                   (0xC40001AF)

/* RSI DA SMC FID */
#define RSI_VDEV_DMA_ENABLE                   (0xC400019C)
#define RSI_VDEV_GET_INFO                     (0xC400019D)
#define RSI_VDEV_VALIDATE_MAPPING             (0xC400019F)
#define RSI_VDEV_DMA_DISABLE                  (0xC40001A4)

/* PSI HVC/SMC IDs
 * PSI (Plane services interface) is a IMPDEF interface between Pn and P0 */
#define PSI_REALM_CONFIG RSI_REALM_CONFIG
#define PSI_P0_CALL      RSI_HOST_CALL
#define PSI_PRINT_MSG    0x88FFFFFF
#define PSI_TEST_SMC     0x88FFFFFE
#define PSI_TEST_HVC     0x88FFFFFD

#endif /* _VAL_RMM_H_ */
