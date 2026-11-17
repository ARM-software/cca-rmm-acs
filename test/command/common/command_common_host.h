 /*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef _CMD_COMMON_HOST_H
#define _CMD_COMMON_HOST_H
#include "val_host_command.h"
#include "val_host_da.h"
#include "tracking_common_host.h"

uint64_t val_host_tracking_none_prep_sequence(val_host_tracking_test_region_ts *region);
uint32_t val_host_tracking_none_cleanup(val_host_tracking_test_region_ts *region);
uint32_t val_host_realm_create_common(val_host_realm_ts *realm);
uint32_t val_host_rec_create_common(val_host_realm_ts *realm, val_host_rec_params_ts *params);
uint64_t g_delegated_prep_sequence(void);
uint64_t g_undelegated_prep_sequence(void);
uint64_t g_unaligned_prep_sequence(uint64_t gran);
uint64_t g_dev_mem_prep_sequence(void);
uint64_t g_outside_of_permitted_pa_prep_sequence(void);
uint64_t g_secure_prep_sequence(void);
uint64_t g_data_prep_sequence(uint64_t rd, uint64_t ipa);
uint64_t ipa_outside_of_permitted_ipa_prep_sequence(void);
uint64_t ipa_protected_unmapped_prep_sequence(void);
uint64_t ipa_protected_assigned_ram_prep_sequence(uint64_t rd);
uint64_t ipa_protected_assigned_empty_prep_sequence(uint64_t rd);
uint64_t ipa_protected_destroyed_prep_sequence(uint64_t rd);
uint64_t ipa_unprotected_unmapped_prep_sequence(void);
uint64_t ipa_unprotected_assinged_prep_sequence(uint64_t rd);
uint64_t ipa_unprotected_unassigned_prep_sequence(uint64_t rd);
uint64_t ipa_protected_unassigned_ram_prep_sequence(uint64_t rd);
uint64_t ipa_protected_unassigned_empty_prep_sequence(uint64_t rd);
uint64_t ipa_protected_aux_assigned_prep_sequence(uint64_t rd, uint64_t rtt_index);
uint64_t g_pa_in_lpa2_range_prep_sequence(void);
uint64_t g_vdev_new_prep_sequence(val_host_pdev_ts *pdev_dev, val_host_vdev_ts *vdev_dev,
                                                                val_host_realm_ts *realm);
uint64_t g_pdev_ready_prep_sequence(val_host_pdev_ts *pdev_dev);
uint64_t g_root_pdev_ready_prep_sequence(val_host_pdev_stream_ts *pdev_stream, uint32_t rp_bdf);
uint64_t g_pdev_stream_connected_prep_sequence(val_host_pdev_ts *pdev_dev,
                                  val_host_pdev_stream_ts *pdev_stream,
                                  uint8_t stream_type);
uint32_t g_pdev_stream_communicate_prep_sequence(
                                  val_host_pdev_ts *pdev_dev,
                                  val_host_pdev_stream_ts *pdev_stream);
uint32_t g_pdev_stream_teardown_sequence(
                                  val_host_pdev_ts *pdev_dev,
                                  val_host_pdev_stream_ts *pdev_stream);
#endif /* _CMD_COMMON_HOST_H */
