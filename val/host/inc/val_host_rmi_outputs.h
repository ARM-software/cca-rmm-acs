/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_HOST_RMI_OUTPUTS_H_
#define _VAL_HOST_RMI_OUTPUTS_H_

#include "val.h"

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

#endif /* _VAL_HOST_RMI_OUTPUTS_H_ */
