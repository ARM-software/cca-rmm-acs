/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_REALM_RSI_H_
#define _VAL_REALM_RSI_H_

#include "val.h"
#include "val_framework.h"
#include "val_mmu.h"
#include "val_smc.h"
#include "val_rmm.h"

#define RSI_HOST_CALL_NR_GPRS 31

#define SET_MEMBER_RSI    SET_MEMBER

#define RSI_ABI_VERSION         ((RSI_ABI_VERSION_MAJOR << 16) | \
                                                 RSI_ABI_VERSION_MINOR)

#define RSI_DEVICE_MEAS_IDS  256

#define RSI_DEVICE_MEAS_PARAMS  256

#define RSI_VDEV_VCA_DIGEST_LEN         U(64)
#define RSI_VDEV_CERT_DIGEST_LEN         U(64)
#define RSI_VDEV_PUBKEY_DIGEST_LEN         U(64)
#define RSI_VDEV_MEAS_DIGEST_LEN         U(64)
#define RSI_VDEV_REPORT_DIGEST_LEN         U(64)

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
    /* RsiDevFlags: Flags */
    SET_MEMBER_RSI(unsigned long flags, 0, 0x8);
    /* UInt64: cert_id*/
    SET_MEMBER_RSI(unsigned long cert_id, 0x8, 0x10);
    /* RsiHashAlgorithm: Algorithm used to generate device digests */
    SET_MEMBER_RSI(unsigned char hash_algo, 0x10, 0x18);
    /* UInt64: Nonce generated on most recent transition to LOCKED state */
    SET_MEMBER_RSI(unsigned long lock_nonce, 0x18, 0x20);
    /* UInt64: Nonce generated on most recent GET_MEASUREMENT request */
    SET_MEMBER_RSI(unsigned long meas_nonce, 0x20, 0x28);
    /* UInt64: Nonce generated on most recent GET_INTERFACE_REPORT request */
    SET_MEMBER_RSI(unsigned long report_nonce, 0x28, 0x30);
    /* UInt64: TDISP version of the device */
    SET_MEMBER_RSI(unsigned long tdisp_version, 0x30, 0x38);
    /* RsiVdevState: State of the device */
    SET_MEMBER_RSI(unsigned char state, 0x38, 0x40);
    /* Bits512: VCA digest */
    SET_MEMBER_RSI(unsigned char vca_digest[RSI_VDEV_VCA_DIGEST_LEN], 0x40, 0x80);
    /* Bits512: Certificate digest */
    SET_MEMBER_RSI(unsigned char cert_digest[RSI_VDEV_CERT_DIGEST_LEN], 0x80, 0xc0);
    /* Bits512: Public key digest */
    SET_MEMBER_RSI(unsigned char pubkey_digest[RSI_VDEV_PUBKEY_DIGEST_LEN], 0xc0, 0x100);
    /* Bits512: Measurement digest */
    SET_MEMBER_RSI(unsigned char meas_digest[RSI_VDEV_MEAS_DIGEST_LEN], 0x100, 0x140);
    /* Bits512: Interface report digest */
    SET_MEMBER_RSI(unsigned char report_digest[RSI_VDEV_REPORT_DIGEST_LEN], 0x140, 0x200);
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

uint64_t val_realm_rsi_version(uint64_t req, val_realm_rsi_version_ts *output);
uint64_t val_realm_rsi_realm_config(uint64_t buff);
uint64_t val_realm_rsi_host_call(uint16_t imm);
val_realm_rsi_host_call_t *val_realm_rsi_host_call_ripas(uint16_t imm);
uint64_t val_realm_rsi_host_call_struct(uint64_t gv_realm_host_call1);
uint64_t val_realm_rsi_rhi_host(val_realm_rsi_host_call_t *rsi_host_call);
uint64_t val_realm_get_ipa_width(void);
val_smc_param_ts val_realm_rsi_ipa_state_set(uint64_t base, uint64_t size, uint8_t ripas,
                                                                         uint64_t flags);
val_smc_param_ts val_realm_rsi_ipa_state_get(uint64_t base, uint64_t top);
uint64_t val_realm_rsi_host_params(val_realm_rsi_host_call_t *realm_host_params);
val_smc_param_ts val_realm_rsi_attestation_token_continue(uint64_t addr, uint64_t offset,
                                                           uint64_t size, uint64_t *len);
val_smc_param_ts val_realm_rsi_attestation_token_init(uint64_t challenge_0,
                 uint64_t challenge_1, uint64_t challenge_2, uint64_t challenge_3,
                 uint64_t challenge_4, uint64_t challenge_5, uint64_t challenge_6,
                                                            uint64_t challenge_7);
uint64_t val_realm_rsi_measurement_extend(uint64_t index, uint64_t size, uint64_t value_0,
                                     uint64_t value_1, uint64_t value_2, uint64_t value_3,
                                     uint64_t value_4, uint64_t value_5, uint64_t value_6,
                                                                        uint64_t value_7);
val_smc_param_ts val_realm_rsi_measurement_read(uint64_t index);
uint64_t val_realm_rsi_features(uint64_t index, uint64_t *value);
val_smc_param_ts val_realm_rsi_mem_get_perm_value(uint64_t plane_index, uint64_t perm_index);
val_smc_param_ts val_realm_rsi_mem_set_perm_index(uint64_t base, uint64_t top,  uint64_t perm_index,
                                                                                uint64_t cookie);
val_smc_param_ts val_realm_rsi_mem_set_perm_value(uint64_t plane_index, uint64_t perm_index,
                                                                                   uint64_t value);
val_smc_param_ts val_realm_rsi_plane_enter(uint64_t plane_idx, uint64_t run_ptr);
val_smc_param_ts val_realm_rsi_plane_reg_read(uint64_t plane_idx, uint64_t encoding);
val_smc_param_ts val_realm_rsi_plane_reg_write(uint64_t plane_idx, uint64_t encoding,
                                                                             uint64_t value);
val_smc_param_ts val_realm_rsi_vdev_dma_disable(uint64_t vdev_id);
val_smc_param_ts val_realm_rsi_vdev_dma_enable(uint64_t vdev_id, uint64_t flags,
                                    uint64_t non_ats_plane, uint64_t lock_nonce,
                                    uint64_t meas_nonce,  uint64_t report_nonce);
val_smc_param_ts val_realm_rsi_vdev_get_info(uint64_t vdev_id, uint64_t addr);
val_smc_param_ts val_realm_rsi_vdev_validate_mapping(uint64_t vdev_id, uint64_t ipa_base,
                                                      uint64_t ipa_top, uint64_t pa_base,
                                                     uint64_t flags, uint64_t lock_nonce,
                                             uint64_t meas_nonce,  uint64_t report_nonce);

#endif /* _VAL_REALM_RMI_H_ */
