/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    PDEV_UNALIGNED = 0X0,
    PDEV_OUTSIDE_OF_PERMITTED_PA = 0X1,
    PDEV_DEV_MEM_MMIO = 0X2,
    PDEV_GRAN_STATE_UNDELEGATED = 0X3,
    PDEV_STATE_PDEV_COMMUNICATING = 0X4,
    PDEV_STATE_PDEV_STOPPING = 0X5,
    PDEV_STATE_PDEV_STOPPED = 0X6,
    VDEV_IS_NON_ZERO = 0X7
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
    .abi = RMI_PDEV_STOP,
    .label = PDEV_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pdev_bound",
    .abi = RMI_PDEV_STOP,
    .label = PDEV_OUTSIDE_OF_PERMITTED_PA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pdev_bound",
    .abi = RMI_PDEV_STOP,
    .label = PDEV_DEV_MEM_MMIO,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pdev_gran_state",
    .abi = RMI_PDEV_STOP,
    .label = PDEV_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pdev_state",
    .abi = RMI_PDEV_STOP,
    .label = PDEV_STATE_PDEV_COMMUNICATING,
    .status = RMI_ERROR_DEVICE,
    .index = 0},
    {.msg = "pdev_state",
    .abi = RMI_PDEV_STOP,
    .label = PDEV_STATE_PDEV_STOPPING,
    .status = RMI_ERROR_DEVICE,
    .index = 0},
    {.msg = "pdev_state",
    .abi = RMI_PDEV_STOP,
    .label = PDEV_STATE_PDEV_STOPPED,
    .status = RMI_ERROR_DEVICE,
    .index = 0},
    {.msg = "num_vdevs",
    .abi = RMI_PDEV_STOP,
    .label = VDEV_IS_NON_ZERO,
    .status = RMI_ERROR_DEVICE,
    .index = 0}
};
