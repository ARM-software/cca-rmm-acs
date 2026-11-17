/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TRACKING_COMMON_HOST_H
#define TRACKING_COMMON_HOST_H

#include "val_host_rmi.h"

#define TRACKING_TEST_REGION_SIZE_2MB (2ULL * 1024 * 1024)

typedef struct {
    uint64_t base;
    uint64_t delegated_start;
    uint64_t delegated_top;
    uint64_t initial_state;
    bool state_changed;
} val_host_tracking_test_region_ts;

uint64_t val_host_tracking_coarse_prep_sequence(val_host_tracking_test_region_ts *region);
uint32_t val_host_tracking_coarse_cleanup(val_host_tracking_test_region_ts *region);
uint32_t val_host_tracking_test_range_transition(uint64_t base, uint64_t top,
                                                 bool delegate,
                                                 uint64_t *completed_top);

#endif /* TRACKING_COMMON_HOST_H */
