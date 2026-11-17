/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "val_rmm.h"

enum test_intent {
    STREAM_INVALID = 0X0,
    STREAM_TYPE_INVALID = 0X1,
    PDEV_1_UNALIGNED = 0X2,
    PDEV_1_GRAN_STATE_UNDELEGATED = 0X3,
    PDEV_2_UNALIGNED = 0X4,
    PDEV_2_GRAN_STATE_UNDELEGATED = 0X5
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "stream_valid",
    .abi = RMI_PDEV_STREAM_KEY_REFRESH,
    .label = STREAM_INVALID,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "stream_type",
    .abi = RMI_PDEV_STREAM_KEY_REFRESH,
    .label = STREAM_TYPE_INVALID,
    .status = RMI_ERROR_DEVICE,
    .index = 0},
    {.msg = "pdev_1_align",
    .abi = RMI_PDEV_STREAM_KEY_REFRESH,
    .label = PDEV_1_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pdev_1_gran_state",
    .abi = RMI_PDEV_STREAM_KEY_REFRESH,
    .label = PDEV_1_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pdev_2_align",
    .abi = RMI_PDEV_STREAM_KEY_REFRESH,
    .label = PDEV_2_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pdev_2_gran_state",
    .abi = RMI_PDEV_STREAM_KEY_REFRESH,
    .label = PDEV_2_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
