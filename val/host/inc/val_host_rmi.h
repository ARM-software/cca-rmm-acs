/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_HOST_RMI_H_
#define _VAL_HOST_RMI_H_

#include "val.h"
#include "val_framework.h"
#include "val_mmu.h"
#include "val_smc.h"
#include "val_host_alloc.h"
#include "val_host_framework.h"
#include "val_host_realm.h"

typedef struct {
    uint64_t walk_level;
    uint64_t state;
    uint64_t desc;
    uint64_t ripas;
} val_host_rtt_entry_ts;

typedef struct {
    uint64_t rtt;
    uint64_t top;
} val_host_rtt_destroy_ts;

typedef struct {
    uint64_t data;
    uint64_t top;
} val_host_data_destroy_ts;

typedef struct {
    uint64_t lower;
    uint64_t higher;
} val_host_rmi_version_ts;

uint64_t val_host_rmi_version(uint64_t req, val_host_rmi_version_ts *output);
uint64_t val_host_rmi_features(uint64_t index, uint64_t *value);
uint64_t val_host_rmi_data_create(uint64_t rd, uint64_t data,
               uint64_t ipa, uint64_t src, uint64_t flags);
uint64_t val_host_rmi_data_create_unknown(uint64_t rd,
                   uint64_t data, uint64_t ipa);
uint64_t val_host_rmi_granule_delegate(uint64_t addr);
uint64_t val_host_rmi_granule_undelegate(uint64_t addr);
uint64_t val_host_rmi_psci_complete(uint64_t calling_rec,
                 uint64_t target_rec, uint64_t status);
uint64_t val_host_rmi_realm_activate(uint64_t rd);
uint64_t val_host_rmi_realm_create(uint64_t rd, uint64_t params_ptr);
uint64_t val_host_rmi_realm_destroy(uint64_t rd);
uint64_t val_host_rmi_data_destroy(uint64_t rd, uint64_t ipa,
                     val_host_data_destroy_ts *data_destroy);
uint64_t val_host_rmi_rec_aux_count(uint64_t rd, uint64_t *aux_count);
uint64_t val_host_rmi_rec_create(uint64_t rd, uint64_t rec,
                 uint64_t params_ptr);
uint64_t val_host_rmi_rec_destroy(uint64_t rec);
uint64_t val_host_rmi_rec_enter(uint64_t rec, uint64_t run_ptr);
uint64_t val_host_rmi_rtt_create(uint64_t rd, uint64_t rtt,
              uint64_t ipa, uint64_t level);
uint64_t val_host_rmi_rtt_fold(uint64_t rd,
               uint64_t ipa, uint64_t level, uint64_t *rtt);
uint64_t val_host_rmi_rtt_destroy(uint64_t rd,
               uint64_t ipa, uint64_t level, val_host_rtt_destroy_ts *rtt_destroy);
uint64_t val_host_rmi_rtt_map_unprotected(uint64_t rd, uint64_t ipa,
                  uint64_t level, uint64_t desc);
uint64_t val_host_rmi_rtt_read_entry(uint64_t rd, uint64_t ipa,
                 uint64_t level, val_host_rtt_entry_ts *rtt);
uint64_t val_host_rmi_rtt_unmap_unprotected(uint64_t rd, uint64_t ipa,
                    uint64_t level, uint64_t *top);
uint64_t val_host_rmi_rtt_init_ripas(uint64_t rd, uint64_t base,
                    uint64_t top, uint64_t *out_top);
uint64_t val_host_rmi_rtt_set_ripas(uint64_t rd, uint64_t rec,
                    uint64_t base, uint64_t top, uint64_t *out_top);
val_smc_param_ts val_host_rmi_rtt_set_s2ap(uint64_t rd, uint64_t rec, uint64_t base, uint64_t top);
val_smc_param_ts val_host_rmi_rtt_aux_create(uint64_t rd, uint64_t rtt,
              uint64_t ipa, uint64_t level, uint64_t index);
val_smc_param_ts val_host_rmi_rtt_aux_destroy(uint64_t rd,
               uint64_t ipa, uint64_t level, uint64_t index);
val_smc_param_ts val_host_rmi_rtt_aux_fold(uint64_t rd,
              uint64_t ipa, uint64_t level, uint64_t index);
val_smc_param_ts val_host_rmi_rtt_aux_map_protected(uint64_t rd, uint64_t ipa, uint64_t index);
val_smc_param_ts val_host_rmi_rtt_aux_map_unprotected(uint64_t rd, uint64_t ipa, uint64_t index);
val_smc_param_ts val_host_rmi_rtt_aux_unmap_protected(uint64_t rd, uint64_t ipa, uint64_t index);
val_smc_param_ts val_host_rmi_rtt_aux_unmap_unprotected(uint64_t rd, uint64_t ipa, uint64_t index);
val_smc_param_ts val_host_rmi_granule_io_delegate(uint64_t addr, uint64_t flags);
val_smc_param_ts val_host_rmi_granule_io_undelegate(uint64_t addr);
val_smc_param_ts val_host_rmi_io_create(uint64_t rd, uint64_t ipa, uint64_t flags, uint64_t desc);
val_smc_param_ts val_host_rmi_io_destroy(uint64_t rd, uint64_t ipa);
val_smc_param_ts val_host_rmi_pdev_abort(uint64_t pdev_ptr);
val_smc_param_ts val_host_rmi_pdev_communicate(uint64_t pdev_ptr, uint64_t data_ptr);
val_smc_param_ts val_host_rmi_pdev_create(uint64_t pdev_ptr, uint64_t params_ptr);
val_smc_param_ts val_host_rmi_pdev_destroy(uint64_t pdev_ptr);
val_smc_param_ts val_host_rmi_pdev_get_state(uint64_t pdev_ptr);
val_smc_param_ts val_host_rmi_pdev_ide_reset(uint64_t pdev_ptr);
val_smc_param_ts val_host_rmi_pdev_notify(uint64_t pdev_ptr, uint64_t ev);
val_smc_param_ts val_host_rmi_pdev_set_key(uint64_t pdev_ptr, uint64_t key,
                                               uint64_t len, uint8_t algo);
val_smc_param_ts val_host_rmi_pdev_stop(uint64_t pdev_ptr);
val_smc_param_ts val_host_rmi_vdev_abort(uint64_t vdev_ptr);
val_smc_param_ts val_host_rmi_vdev_communicate(uint64_t vdev_ptr, uint64_t data_ptr);
val_smc_param_ts val_host_rmi_vdev_create(uint64_t rd, uint64_t pdev_ptr,
                                  uint64_t vdev_ptr, uint64_t params_ptr);
val_smc_param_ts val_host_rmi_vdev_destroy(uint64_t vdev_ptr);
val_smc_param_ts val_host_rmi_vdev_get_state(uint64_t vdev_ptr);
val_smc_param_ts val_host_rmi_vdev_stop(uint64_t vdev_ptr);
val_smc_param_ts val_host_rmi_mec_set_private(uint64_t mecid);
val_smc_param_ts val_host_rmi_mec_set_shared(uint64_t mecid);

#endif /* _VAL_HOST_RMI_H_ */
