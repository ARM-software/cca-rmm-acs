/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
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
#include "val_host_rmi_types.h"
#include "val_host_rmi_outputs.h"
#include "val_host_rmi_sro.h"
#include "val_host_alloc.h"
#include "val_host_framework.h"
#include "val_host_realm.h"

uint64_t val_host_rmi_version(uint64_t req, val_host_rmi_version_ts *output);
uint64_t val_host_rmi_features(uint64_t index, uint64_t *value);
uint64_t val_host_rmi_data_create(uint64_t rd, uint64_t data,
               uint64_t ipa, uint64_t src, uint64_t flags);
val_smc_param_ts val_host_rmi_rtt_data_map_init_smc(uint64_t rd, uint64_t data,
                  uint64_t ipa, uint64_t src, uint64_t flags);
uint64_t val_host_rmi_data_create_unknown(uint64_t rd,
                   uint64_t data, uint64_t ipa);
uint64_t val_host_rmi_granule_delegate(uint64_t addr);
uint64_t val_host_rmi_granule_undelegate(uint64_t addr);
uint64_t val_host_rmi_psci_complete(uint64_t calling_rec, uint64_t status);
uint64_t val_host_rmi_realm_activate(uint64_t rd);
val_smc_param_ts val_host_rmi_realm_create_smc(uint64_t rd, uint64_t params_ptr);
val_smc_param_ts val_host_rmi_realm_destroy_smc(uint64_t rd);
val_smc_param_ts val_host_rmi_realm_terminate_smc(uint64_t rd);
uint64_t val_host_rmi_data_destroy(uint64_t rd, uint64_t ipa,
                     val_host_data_destroy_ts *data_destroy);
uint64_t val_host_rmi_rec_aux_count(uint64_t rd, uint64_t *aux_count);
val_smc_param_ts val_host_rmi_rec_create_smc(uint64_t rd, uint64_t rec,
                 uint64_t params_ptr);
val_smc_param_ts val_host_rmi_rec_destroy_smc(uint64_t rec);
uint64_t val_host_rmi_rec_enter(uint64_t rec, uint64_t run_ptr);
val_smc_param_ts val_host_rmi_rtt_create_smc(uint64_t rd, uint64_t rtt,
                  uint64_t ipa, uint64_t level);
val_smc_param_ts val_host_rmi_rtt_destroy_smc(uint64_t rd,
                  uint64_t ipa, uint64_t level);
val_smc_param_ts val_host_rmi_rtt_fold_smc(uint64_t rd,
                  uint64_t ipa, uint64_t level);
uint64_t val_host_rmi_rtt_map_unprotected(uint64_t rd, uint64_t ipa,
                  uint64_t level, uint64_t desc);
uint64_t val_host_rmi_rtt_map_protected(uint64_t rd, uint64_t ipa,
                  uint64_t level, uint64_t desc);
uint64_t val_host_rmi_rtt_read_entry(uint64_t rd, uint64_t ipa,
                 uint64_t level, val_host_rtt_entry_ts *rtt);
uint64_t val_host_rmi_rtt_unmap_unprotected(uint64_t rd, uint64_t ipa,
                    uint64_t level, uint64_t *top);
val_smc_param_ts val_host_rmi_rtt_init_ripas_smc(uint64_t rd, uint64_t base,
                        uint64_t top, uint64_t *out_top);
val_smc_param_ts val_host_rmi_rtt_set_ripas_smc(uint64_t rd, uint64_t rec,
                    uint64_t base, uint64_t top, uint64_t *out_top);
val_smc_param_ts val_host_rmi_rmm_config_set(uint64_t cfg_ptr);
val_smc_param_ts val_host_rmi_granule_tracking_get(uint64_t base, uint64_t top);
val_smc_param_ts val_host_rmi_granule_tracking_set_smc(uint64_t addr,
                    uint64_t category, uint64_t state);
val_smc_param_ts val_host_rmi_rmm_config_get(uint64_t cfg_ptr);
val_smc_param_ts val_host_rmi_rmm_state_get(void);
val_smc_param_ts val_host_rmi_granule_range_delegate_smc(uint64_t base, uint64_t top);
val_smc_param_ts val_host_rmi_granule_range_undelegate_smc(uint64_t base, uint64_t top);
val_smc_param_ts val_host_rmi_gpt_l1_create_smc(uint64_t addr);
val_smc_param_ts val_host_rmi_gpt_l1_destroy_smc(uint64_t addr);
val_smc_param_ts val_host_rmi_rtt_data_map_smc(uint64_t rd, uint64_t base,
                    uint64_t top, uint64_t flags, uint64_t oaddr);
val_smc_param_ts val_host_rmi_rtt_data_unmap_smc(uint64_t rd, uint64_t base,
                    uint64_t top, uint64_t flags, uint64_t oaddr);
val_smc_param_ts val_host_rmi_rtt_unprot_map_smc(uint64_t rd, uint64_t base,
                    uint64_t top, uint64_t flags, uint64_t oaddr);
val_smc_param_ts val_host_rmi_rtt_unprot_unmap_smc(uint64_t rd, uint64_t base,
                    uint64_t top, uint64_t flags, uint64_t oaddr);
val_smc_param_ts val_host_rmi_rmm_activate_smc(void);
val_smc_param_ts val_host_rmi_op_continue_smc(uint64_t handle, uint64_t flags);
val_smc_param_ts val_host_rmi_op_mem_donate_smc(uint64_t handle, uint64_t list_addr,
                    uint64_t list_count);
val_smc_param_ts val_host_rmi_op_mem_reclaim_smc(uint64_t handle, uint64_t list_addr,
                    uint64_t list_count);
val_smc_param_ts val_host_rmi_op_cancel_smc(uint64_t handle);
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
val_smc_param_ts val_host_rmi_mec_set_private(uint64_t mecid);
val_smc_param_ts val_host_rmi_mec_set_shared(uint64_t mecid);
val_smc_param_ts val_host_rmi_pdev_abort(uint64_t pdev_ptr);
val_smc_param_ts val_host_rmi_pdev_aux_count(uint64_t flags);
val_smc_param_ts val_host_rmi_pdev_communicate(uint64_t pdev_ptr, uint64_t data_ptr);
val_smc_param_ts val_host_rmi_pdev_create_smc(uint64_t pdev_ptr, uint64_t params_ptr);
val_smc_param_ts val_host_rmi_pdev_destroy_smc(uint64_t pdev_ptr);
val_smc_param_ts val_host_rmi_pdev_get_state(uint64_t pdev_ptr);
val_smc_param_ts val_host_rmi_pdev_ide_reset(uint64_t pdev_ptr);
val_smc_param_ts val_host_rmi_pdev_ide_key_refresh(uint64_t pdev_ptr, uint8_t coh);
val_smc_param_ts val_host_rmi_pdev_set_pubkey(uint64_t pdev_ptr, uint64_t params_ptr);
val_smc_param_ts val_host_rmi_pdev_stop(uint64_t pdev_ptr);
val_smc_param_ts val_host_rmi_pdev_stream_key_refresh(uint64_t pdev_1_ptr,
                    uint64_t pdev_2_ptr, uint64_t stream_hnd);
val_smc_param_ts val_host_rmi_pdev_stream_connect(uint64_t params_ptr);
val_smc_param_ts val_host_rmi_pdev_stream_disconnect(uint64_t pdev_1_ptr,
                    uint64_t pdev_2_ptr, uint64_t stream_hnd);
val_smc_param_ts val_host_rmi_pdev_stream_complete(uint64_t pdev_1_ptr,
                    uint64_t pdev_2_ptr, uint64_t stream_hnd);
val_smc_param_ts val_host_rmi_pdev_stream_key_purge(uint64_t pdev_1_ptr,
                    uint64_t pdev_2_ptr, uint64_t stream_hnd);
val_smc_param_ts val_host_rmi_vdev_abort(uint64_t rd, uint64_t pdev_ptr, uint64_t vdev_ptr);
val_smc_param_ts val_host_rmi_vdev_aux_count(uint64_t pdev_flags, uint64_t vdev_flags);
val_smc_param_ts val_host_rmi_vdev_communicate(uint64_t rd, uint64_t pdev_ptr,
                           uint64_t vdev_ptr, uint64_t data_ptr);
val_smc_param_ts val_host_rmi_vdev_create_smc(uint64_t rd, uint64_t pdev_ptr,
                                  uint64_t vdev_ptr, uint64_t params_ptr);
val_smc_param_ts val_host_rmi_vdev_destroy_smc(uint64_t rd, uint64_t pdev_ptr,
                                                       uint64_t vdev_ptr);
val_smc_param_ts val_host_rmi_vdev_get_state(uint64_t vdev_ptr);
val_smc_param_ts val_host_rmi_vdev_get_interface_report(uint64_t rd, uint64_t pdev_ptr,
                                                                     uint64_t vdev_ptr);
val_smc_param_ts val_host_rmi_vdev_get_measurements(uint64_t rd, uint64_t pdev_ptr,
                                            uint64_t vdev_ptr, uint64_t params_ptr);
val_smc_param_ts val_host_rmi_vdev_lock(uint64_t rd, uint64_t pdev_ptr, uint64_t vdev_ptr);
val_smc_param_ts val_host_rmi_vdev_start(uint64_t rd, uint64_t pdev_ptr, uint64_t vdev_ptr);
val_smc_param_ts val_host_rmi_vdev_unlock(uint64_t rd, uint64_t pdev_ptr, uint64_t vdev_ptr);
val_smc_param_ts val_host_rmi_rtt_dev_validate_smc(uint64_t rd, uint64_t rec_ptr,
                                                   uint64_t base, uint64_t top);
val_smc_param_ts val_host_rmi_rtt_dev_map_smc(uint64_t rd, uint64_t vdev_ptr,
                    uint64_t base, uint64_t top, uint64_t flags, uint64_t oaddr);
val_smc_param_ts val_host_rmi_rtt_dev_unmap_smc(uint64_t rd, uint64_t base,
                    uint64_t top, uint64_t flags, uint64_t oaddr);
val_smc_param_ts val_host_rmi_psmmu_irq_notify(uint64_t psmmu_ptr, uint64_t irqs);
val_smc_param_ts val_host_rmi_psmmu_activate_smc(uint64_t psmmu_ptr, uint64_t params_ptr);
val_smc_param_ts val_host_rmi_psmmu_deactivate_smc(uint64_t psmmu_ptr);
val_smc_param_ts val_host_rmi_psmmu_st_l2_create_smc(uint64_t psmmu_ptr, uint64_t sid);
val_smc_param_ts val_host_rmi_psmmu_st_l2_destroy_smc(uint64_t psmmu_ptr, uint64_t sid);
val_smc_param_ts val_host_rmi_dpt_l0_create(void);
val_smc_param_ts val_host_rmi_dpt_l0_destroy(void);
val_smc_param_ts val_host_rmi_dpt_l1_create(uint64_t addr);
val_smc_param_ts val_host_rmi_dpt_l1_destroy(uint64_t addr);
val_smc_param_ts val_host_rmi_psmmu_event_consume(uint64_t psmmu_ptr, uint64_t irq);
val_smc_param_ts val_host_rmi_psmmu_info(uint64_t psmmu_ptr, uint64_t info_ptr);

#endif /* _VAL_HOST_RMI_H_ */
