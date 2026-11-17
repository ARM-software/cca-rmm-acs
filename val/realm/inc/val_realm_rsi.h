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
#include "val_realm_rsi_types.h"

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
                                    uint64_t non_ats_plane, uint64_t lock_seq,
                                    uint64_t meas_seq,  uint64_t report_seq);
val_smc_param_ts val_realm_rsi_vdev_get_info(uint64_t vdev_id, uint64_t addr);
val_smc_param_ts val_realm_rsi_vdev_validate_mapping(uint64_t vdev_id, uint64_t ipa_base,
                                                      uint64_t ipa_top, uint64_t pa_base,
                                                     uint64_t flags, uint64_t lock_seq,
                                             uint64_t meas_seq,  uint64_t report_seq);

#endif /* _VAL_REALM_RSI_H_ */
