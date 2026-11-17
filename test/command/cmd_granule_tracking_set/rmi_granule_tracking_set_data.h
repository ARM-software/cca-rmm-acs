/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "val_rmm.h"

enum test_intent {
    STATE_INVALID = 0X0,
    ADDR_UNALIGNED = 0X1,
    ADDR_BOUND = 0X2,
    CATEGORY_INCOMPATIBLE = 0X3
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "state_valid",
    .abi = RMI_GRANULE_TRACKING_SET,
    .label = STATE_INVALID,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "addr_align",
    .abi = RMI_GRANULE_TRACKING_SET,
    .label = ADDR_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "addr_bound",
    .abi = RMI_GRANULE_TRACKING_SET,
    .label = ADDR_BOUND,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "category",
    .abi = RMI_GRANULE_TRACKING_SET,
    .label = CATEGORY_INCOMPATIBLE,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
