/*
 * Copyright (c) 2023, 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    BASE_UNALIGNED = 0X0,
    TOP_UNALIGNED = 0X1,
    TOP_BOUND = 0X2,
    POPULATED = 0X3,
    TRACKING_NONE = 0X4,
    STATE_RD = 0X5
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "base_align",
    .abi = RMI_GRANULE_RANGE_DELEGATE,
    .label = BASE_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "top_align",
    .abi = RMI_GRANULE_RANGE_DELEGATE,
    .label = TOP_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "top_bound",
    .abi = RMI_GRANULE_RANGE_DELEGATE,
    .label = TOP_BOUND,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "populated",
    .abi = RMI_GRANULE_RANGE_DELEGATE,
    .label = POPULATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "tracking",
    .abi = RMI_GRANULE_RANGE_DELEGATE,
    .label = TRACKING_NONE,
    .status = RMI_ERROR_TRACKING,
    .index = 0},
    {.msg = "state",
    .abi = RMI_GRANULE_RANGE_DELEGATE,
    .label = STATE_RD,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
