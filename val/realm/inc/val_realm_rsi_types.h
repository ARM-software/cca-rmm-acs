/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_REALM_RSI_TYPES_H_
#define _VAL_REALM_RSI_TYPES_H_

#include "val.h"

/* RsiInterfaceVersion type */
#define RSI_MAJOR_VERSION    0
#define RSI_MINOR_VERSION    0

/* RsiStatusCode types */
#define RSI_SUCCESS                0
#define RSI_ERROR_INPUT            1
#define RSI_ERROR_STATE            2
#define RSI_ERROR_INCOMPLETE       3
#define RSI_INCOMPLETE             3
#define RSI_ERROR_UNKNOWN          4
#define RSI_ERROR_DEVICE           5

/* RsiHashAlgorithm type */
#define RSI_HASH_SHA_256 0
#define RSI_HASH_SHA_512 1
#define RSI_HASH_SHA_384 2

/* RsiRipas type */
#define RSI_EMPTY        0
#define RSI_RAM          1
#define RSI_DESTROYED    2
#define RSI_DEV          3

/* RsiRipasChangeDestroyed type */
#define RSI_NO_CHANGE_DESTROYED     0
#define RSI_CHANGE_DESTROYED        1

/* RsiResponse types */
#define RSI_ACCEPT                 0
#define RSI_REJECT                 1

/* RsiGicOwner type */
#define RSI_GIC_OWNER_0            0
#define RSI_GIC_OWNER_N            1

/* RsiPlaneExitReason types */
#define RSI_EXIT_SYNC              0

/* RsiDevMemCoherent type */
#define RSI_DEV_MEM_NON_COHERENT      0
#define RSI_DEV_MEM_COHERENT          1

/* RsiDevMemOrdering type */
#define RSI_DEV_MEM_NOT_LIMITED_ORDER      0
#define RSI_DEV_MEM_LIMITED_ORDER          1

/* RsiBoolean type */
#define RSI_FALSE                  0
#define RSI_TRUE                   1

/* RsiVdevState type */
#define RSI_VDEV_UNLOCKED      0
#define RSI_VDEV_LOCKED        1
#define RSI_VDEV_STARTED       2
#define RSI_VDEV_ERROR         3

/* RsiFeature type */
#define RSI_FEATURE_FALSE           0
#define RSI_FEATURE_TRUE            1

/* RsiTrap type */
#define RSI_NO_TRAP                 0
#define RSI_TRAP                    1

#define RSI_HOST_CALL_NR_GPRS 31

#define SET_MEMBER_RSI    SET_MEMBER

#define RSI_ABI_VERSION         ((RSI_ABI_VERSION_MAJOR << 16) | \
                                                 RSI_ABI_VERSION_MINOR)

#define RSI_VDEV_PROTOCOL_DATA_DIGEST_LEN U(64)
#define RSI_VDEV_IDENTITY_DIGEST_LEN      U(64)
#define RSI_VDEV_PUBKEY_DIGEST_LEN        U(64)
#define RSI_VDEV_MEAS_DIGEST_LEN          U(64)
#define RSI_VDEV_REPORT_DIGEST_LEN        U(64)

typedef struct _val_realm_rsi_host_call {
    SET_MEMBER_RSI(struct {
        /* Immediate value */
        unsigned int imm;        /* Offset 0 */
        /* Registers */
        unsigned long gprs[RSI_HOST_CALL_NR_GPRS];
        }, 0, 0x100);
} val_realm_rsi_host_call_t;

typedef struct __attribute__((packed)) {
    uint8_t destroyed:1;
    uint64_t unused:63;
} val_realm_ripas_change_flags_ts;

typedef struct {
    /* IPA width in bits */
    SET_MEMBER_RSI(unsigned long ipa_width, 0, 8);
    /* Hash algorithm */
    SET_MEMBER_RSI(unsigned char hash_algo, 0x8, 0x10);
    /* Number of auxiliary Planes */
    SET_MEMBER_RSI(unsigned long num_aux_planes, 0x10, 0x18);
    /* GICv3 VGIC Type Register value */
    SET_MEMBER_RSI(unsigned long gicv3_vtr, 0x18, 0x20);
    /*
     * If ATS is enabled, determines the stage 2 translation used by devices
     * assigned to the Realm
     */
    SET_MEMBER_RSI(unsigned long ats_plane, 0x20, 0x200);

    /* Realm Personalization Value */
    SET_MEMBER_RSI(unsigned char rpv[64], 0x200, 0x1000);
} val_realm_rsi_realm_config_ts;

typedef struct {
    uint64_t lower;
    uint64_t higher;
} val_realm_rsi_version_ts;

typedef struct {
    /* RsiVdevFlags: Flags */
    SET_MEMBER_RSI(unsigned long flags, 0, 0x8);
    /* UInt64: Device identity index */
    SET_MEMBER_RSI(unsigned long id_index, 0x8, 0x10);
    /* RsiHashAlgorithm: Algorithm used to generate device digests */
    SET_MEMBER_RSI(unsigned char hash_algo, 0x10, 0x18);
    /* UInt64: Nonce generated on most recent transition to LOCKED state */
    SET_MEMBER_RSI(unsigned long lock_seq, 0x18, 0x20);
    /* UInt64: Nonce generated on most recent GET_MEASUREMENT request */
    SET_MEMBER_RSI(unsigned long meas_seq, 0x20, 0x28);
    /* UInt64: Nonce generated on most recent GET_INTERFACE_REPORT request */
    SET_MEMBER_RSI(unsigned long report_seq, 0x28, 0x30);
    /* RsiVdevReportFormatType: Report format type */
    SET_MEMBER_RSI(unsigned char format_type, 0x30, 0x38);
    /* Bits64: Report format version */
    SET_MEMBER_RSI(unsigned long format_version, 0x38, 0x40);
    /* RsiVdevState: State of the device */
    SET_MEMBER_RSI(unsigned char state, 0x40, 0x80);
    /* Bits512: Protocol data digest */
    SET_MEMBER_RSI(unsigned char protocol_data_digest[RSI_VDEV_PROTOCOL_DATA_DIGEST_LEN],
                   0x80, 0xc0);
    /* Bits512: Device identity digest */
    SET_MEMBER_RSI(unsigned char identity_digest[RSI_VDEV_IDENTITY_DIGEST_LEN], 0xc0, 0x100);
    /* Bits512: Public key digest */
    SET_MEMBER_RSI(unsigned char pubkey_digest[RSI_VDEV_PUBKEY_DIGEST_LEN], 0x100, 0x140);
    /* Bits512: Measurement digest */
    SET_MEMBER_RSI(unsigned char meas_digest[RSI_VDEV_MEAS_DIGEST_LEN], 0x140, 0x180);
    /* Bits512: Interface report digest */
    SET_MEMBER_RSI(unsigned char report_digest[RSI_VDEV_REPORT_DIGEST_LEN], 0x180, 0x1c0);
    /* Address: Base IPA of the associated VSMMU */
    SET_MEMBER_RSI(unsigned long vsmmu_addr, 0x1c0, 0x1c8);
    /* Bits64: Virtual Stream ID */
    SET_MEMBER_RSI(unsigned long vsmmu_vsid, 0x1c8, 0x200);
} val_realm_rsi_vdev_info_ts;


typedef struct __attribute__((packed)) {
    uint8_t DA:1;
    uint8_t MRO:1;
    uint8_t ATS:1;
    uint64_t unused:61;
} val_realm_rsi_feature_register0_ts;


typedef struct {
    /* Properties of device measurements */
    SET_MEMBER_RSI(unsigned long flags, 0, 0x8);
    /* Measurement indices */
    SET_MEMBER_RSI(unsigned char indices[32], 0x100, 0x200);
    /* Nonce value used in measurement requests */
    SET_MEMBER_RSI(unsigned char nonce[32], 0x200, 0x1000);
} val_realm_rsi_dev_measure_params_ts;

typedef struct __attribute__((packed)) {
    uint8_t coh:1;
    uint8_t order:1;
    uint64_t unused:62;
} val_realm_rsi_dev_mem_flags_ts;

typedef struct __attribute__((packed)) {
    uint8_t ats:1;
    uint64_t unused:63;
} val_realm_rsi_vdev_dma_flags_ts;

#endif /* _VAL_REALM_RSI_TYPES_H_ */
