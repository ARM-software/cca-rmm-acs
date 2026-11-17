/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    PDEV_UNALIGNED = 0X0,
    PDEV_GRAN_STATE_UNDELEGATED = 0X2,
    PDEV_STATE_PDEV_NEW = 0X3,
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "pdev_align",
    .abi = RMI_PDEV_DESTROY,
    .label = PDEV_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pdev_gran_state",
    .abi = RMI_PDEV_DESTROY,
    .label = PDEV_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pdev_state",
    .abi = RMI_PDEV_DESTROY,
    .label = PDEV_STATE_PDEV_NEW,
    .status = RMI_ERROR_DEVICE,
    .index = 0}
};
