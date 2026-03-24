/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_HOST_RHI_H_
#define _VAL_HOST_RHI_H_

#include "val.h"
#include "val_framework.h"
#include "val_mmu.h"
#include "val_smc.h"
#include "val_host_alloc.h"
#include "val_host_framework.h"
#include "val_host_realm.h"
#include "val_host_da.h"

typedef struct val_host_realm val_host_realm_ts;
extern uint32_t g_da_vdev_count;
uint64_t val_host_rhi_dispatch(val_host_realm_ts *realm);
uint64_t val_rhi_da_vdev_get_interface_report(val_host_realm_ts *realm,
                                              val_host_vdev_ts *vdev_obj);
uint64_t val_rhi_da_vdev_continue(val_host_realm_ts *realm,
                                  val_host_pdev_ts *pdev_obj, val_host_vdev_ts *vdev_obj);
uint64_t val_rhi_da_vdev_get_measurements(val_host_realm_ts *realm,
                                          val_host_vdev_ts *vdev_obj);
uint64_t val_rhi_da_object_size(val_host_realm_ts *realm,
                                val_host_pdev_ts *pdev_obj,
                                val_host_vdev_ts *vdev_obj);
uint64_t val_rhi_da_object_read(val_host_realm_ts *realm,
                                val_host_pdev_ts *pdev_obj,
                                val_host_vdev_ts *vdev_obj);

uint64_t val_host_rhi_da_dispatch(val_host_realm_ts *realm,
                                  val_host_pdev_ts *pdev_obj, val_host_vdev_ts *vdev_obj);

uint64_t val_rhi_da_vdev_abort(val_host_realm_ts *realm,
                               val_host_vdev_ts *vdev_obj);

uint64_t val_rhi_da_vdev_p2p_unbind(val_host_realm_ts *realm,
                                    val_host_pdev_ts *pdev_obj,
                                    val_host_vdev_ts *vdev_obj);

uint64_t da_init(val_host_realm_ts *realm, val_host_pdev_ts *pdev_obj,
                  val_host_vdev_ts *vdev_obj, uint8_t vdev_state);

uint64_t da_create_vdev(val_host_realm_ts *realm, val_host_pdev_ts *pdev_obj,
                        val_host_vdev_ts *vdev_obj, uint64_t vdev_id,
                        uint8_t vdev_state);

uint64_t val_rhi_da_vdev_set_tdi_state(val_host_realm_ts *realm,
                                      val_host_vdev_ts *vdev_obj);

#endif /* _VAL_HOST_RHI_H_ */
