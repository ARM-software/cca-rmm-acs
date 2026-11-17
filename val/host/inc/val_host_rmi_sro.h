/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_HOST_RMI_SRO_H_
#define _VAL_HOST_RMI_SRO_H_

#include "val.h"
#include "val_host_rmi_outputs.h"

/* SRO RMI ABI wrappers. */
uint64_t val_host_rmi_granule_range_delegate(uint64_t base, uint64_t top);
uint64_t val_host_rmi_granule_range_undelegate(uint64_t base, uint64_t top);
uint64_t val_host_rmi_granule_tracking_set(uint64_t addr,
                                           uint64_t category,
                                           uint64_t state);
uint64_t val_host_rmi_gpt_l1_create(uint64_t addr);
uint64_t val_host_rmi_gpt_l1_destroy(uint64_t addr);
uint64_t val_host_rmi_rmm_activate(void);
uint64_t val_host_rmi_realm_create(uint64_t rd, uint64_t params_ptr);
uint64_t val_host_rmi_pdev_create(uint64_t pdev_ptr, uint64_t params_ptr);
uint64_t val_host_rmi_pdev_destroy(uint64_t pdev_ptr);
uint64_t val_host_rmi_psmmu_activate(uint64_t psmmu_ptr, uint64_t params_ptr);
uint64_t val_host_rmi_psmmu_deactivate(uint64_t psmmu_ptr);
uint64_t val_host_rmi_psmmu_st_l2_create(uint64_t psmmu_ptr, uint64_t sid);
uint64_t val_host_rmi_psmmu_st_l2_destroy(uint64_t psmmu_ptr, uint64_t sid);
uint64_t val_host_rmi_vdev_create(uint64_t rd, uint64_t pdev_ptr,
                                  uint64_t vdev_ptr, uint64_t params_ptr);
uint64_t val_host_rmi_vdev_destroy(uint64_t rd, uint64_t pdev_ptr,
                                   uint64_t vdev_ptr);
uint64_t val_host_rmi_realm_destroy(uint64_t rd);
uint64_t val_host_rmi_rec_create(uint64_t rd, uint64_t rec,
                 uint64_t params_ptr);
uint64_t val_host_rmi_rec_destroy(uint64_t rec);
uint64_t val_host_rmi_rtt_create(uint64_t rd, uint64_t rtt, uint64_t ipa,
                                 uint64_t level);
uint64_t val_host_rmi_rtt_fold(uint64_t rd, uint64_t ipa, uint64_t level,
                               uint64_t *rtt);
uint64_t val_host_rmi_rtt_destroy(uint64_t rd, uint64_t ipa, uint64_t level,
                                  val_host_rtt_destroy_ts *rtt_destroy);
uint64_t val_host_rmi_rtt_data_map_init(uint64_t rd, uint64_t data,
                                        uint64_t ipa, uint64_t src,
                                        uint64_t flags);
uint64_t val_host_rmi_rtt_data_map(uint64_t rd, uint64_t base,
                                   uint64_t top, uint64_t flags,
                                   uint64_t oaddr);
uint64_t val_host_rmi_rtt_data_unmap(uint64_t rd, uint64_t base,
                                     uint64_t top, uint64_t flags,
                                     uint64_t oaddr);
uint64_t val_host_rmi_rtt_dev_map(uint64_t rd, uint64_t vdev_ptr,
                                  uint64_t base, uint64_t top,
                                  uint64_t flags, uint64_t oaddr);
uint64_t val_host_rmi_rtt_dev_unmap(uint64_t rd, uint64_t base,
                                    uint64_t top, uint64_t flags,
                                    uint64_t oaddr);
uint64_t val_host_rmi_rtt_dev_validate(uint64_t rd, uint64_t rec_ptr,
                                       uint64_t base, uint64_t top,
                                       uint64_t *out_top);
uint64_t val_host_rmi_rtt_init_ripas(uint64_t rd, uint64_t base, uint64_t top,
                                     uint64_t *out_top);
uint64_t val_host_rmi_rtt_set_ripas(uint64_t rd, uint64_t rec, uint64_t base,
                                    uint64_t top, uint64_t *out_top);
uint64_t val_host_rmi_rtt_unprot_map(uint64_t rd, uint64_t base,
                                     uint64_t top, uint64_t flags, uint64_t oaddr);
uint64_t val_host_rmi_rtt_unprot_unmap(uint64_t rd, uint64_t base,
                                       uint64_t top, uint64_t flags, uint64_t oaddr);
uint64_t val_host_rmi_realm_terminate(uint64_t rd);

#endif /* _VAL_HOST_RMI_SRO_H_ */
