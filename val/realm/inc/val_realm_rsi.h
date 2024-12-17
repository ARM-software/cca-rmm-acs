/*
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
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
    SET_MEMBER_RSI(unsigned long ipa_width, 0, 8);        /* Offset 0 */
    /* Hash algorithm */
    SET_MEMBER_RSI(unsigned long hash_algo, 8, 0x1000);    /* Offset 8 */
} val_realm_rsi_realm_config_ts;

typedef struct {
    uint64_t lower;
    uint64_t higher;
} val_realm_rsi_version_ts;

typedef struct {
    /* Certificate identifier */
    SET_MEMBER_RSI(unsigned long cert_id, 0, 0x40);     /* Offset 0 */
    /* Certificate digest */
    SET_MEMBER_RSI(unsigned long cert_digest, 0x40, 0x80);  /* Offset 40 */
    /* Device public key digest */
    SET_MEMBER_RSI(unsigned long key_digest, 0x80, 0xC0);     /* Offset 80 */
    /* Measurement block digest */
    SET_MEMBER_RSI(unsigned long meas_digest, 0xC0, 0x100);  /* Offset C0 */
    /* Interface report digest */
    SET_MEMBER_RSI(unsigned long report_digest, 0x100, 0x200);     /* Offset 100 */
} val_realm_rsi_device_digests_ts;

typedef struct {
    /* Measurement indic */
    SET_MEMBER_RSI(unsigned long meas_ids[RSI_DEVICE_MEAS_IDS], 0x0, 0x20);        /* Offset 0 */
    /* Measurement parameters */
    SET_MEMBER_RSI(unsigned long meas_params[RSI_DEVICE_MEAS_PARAMS], 0x20, 0x40); /* Offset 20 */
} val_realm_rsi_device_mesaurement_params_ts;

typedef struct __attribute__((packed)) {
    uint8_t DA:1;
    uint8_t MRO:1;
    uint64_t unused:62;
} val_realm_rsi_feature_register0_ts;

typedef struct __attribute__((packed)) {
    uint8_t share:1;
    uint64_t unused:63;
} val_realm_rsi_rdev_validate_io_flags_ts;

uint64_t val_realm_rsi_version(uint64_t req, val_realm_rsi_version_ts *output);
uint64_t val_realm_rsi_realm_config(uint64_t buff);
uint64_t val_realm_rsi_host_call(uint16_t imm);
val_realm_rsi_host_call_t *val_realm_rsi_host_call_ripas(uint16_t imm);
uint64_t val_realm_rsi_host_call_struct(uint64_t gv_realm_host_call1);
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
val_smc_param_ts val_realm_rsi_rdev_continue(uint64_t dev_id);
val_smc_param_ts val_realm_rsi_rdev_get_digests(uint64_t dev_id, uint64_t addr);
val_smc_param_ts val_realm_rsi_rdev_get_interface_report(uint64_t dev_id, uint64_t version_max);
val_smc_param_ts val_realm_rsi_rdev_get_measurements(uint64_t dev_id, uint64_t params_ptr);
val_smc_param_ts val_realm_rsi_rdev_get_state(uint64_t dev_id);
val_smc_param_ts val_realm_rsi_rdev_lock(uint64_t dev_id);
val_smc_param_ts val_realm_rsi_rdev_start(uint64_t dev_id);
val_smc_param_ts val_realm_rsi_rdev_stop(uint64_t dev_id);
val_smc_param_ts val_realm_rsi_rdev_validate_io(uint64_t dev_id, uint64_t ipa_base,
                       uint64_t ipa_top, uint64_t pa_base, uint64_t flags);

#endif /* _VAL_REALM_RMI_H_ */
