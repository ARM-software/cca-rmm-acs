/*
 * Copyright (c) 2023, 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __MM_COMMON_HOST__
#define __MM_COMMON_HOST__

#include "test_database.h"
#include "val_host_framework.h"
#include "val_host_rmi.h"

typedef struct {
    val_host_realm_ts realm;
    uint64_t rec;
    uint64_t handle;
    uint64_t sro_active;
    uint64_t rec_destroyed;
    val_smc_param_ts args;
} mm_reclaim_sro_ts;

typedef struct {
    val_host_realm_ts realm;
    uint64_t params_addr;
    uint64_t rd_delegated;
    uint64_t rtt_delegated;
    uint64_t realm_created;
} mm_realm_create_sro_ts;

typedef struct {
    uint64_t list_addr;
    uint64_t mem_base;
    uint64_t mem_size;
    uint64_t mem_delegated;
    uint64_t mem_consumed;
} mm_sro_donation_ts;

uint32_t val_host_init_ripas_delegate(val_host_realm_ts *realm,
                                            val_data_create_ts *data_create);
uint64_t mm_prepare_rec_destroy_reclaim_sro(mm_reclaim_sro_ts *sro);
uint64_t mm_reclaim_sro_once(mm_reclaim_sro_ts *sro,
                             uint64_t list_addr,
                             uint64_t list_count);
uint64_t mm_complete_reclaim_sro(mm_reclaim_sro_ts *sro);
void mm_cleanup_reclaim_sro(mm_reclaim_sro_ts *sro);
uint64_t mm_prepare_realm_create_sro(mm_realm_create_sro_ts *sro);
void mm_cleanup_realm_create_sro(mm_realm_create_sro_ts *sro);
uint64_t mm_prepare_sro_donation_list(uint64_t donate_req,
                                      mm_sro_donation_ts *donation);
void mm_release_sro_donation_list(mm_sro_donation_ts *donation);

#endif /* #ifndef __MM_COMMON_HOST__ */
