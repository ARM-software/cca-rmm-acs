/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    VDEV_UNALIGNED = 0X0,
    VDEV_OUTSIDE_OF_PERMITTED_PA = 0X1,
    VDEV_DEV_MEM_MMIO = 0X2,
    VDEV_GRAN_STATE_UNDELEGATED = 0X3
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "vdev_align",
    .abi = RMI_VDEV_GET_STATE,
    .label = VDEV_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "vdev_bound",
    .abi = RMI_VDEV_GET_STATE,
    .label = VDEV_OUTSIDE_OF_PERMITTED_PA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "vdev_bound",
    .abi = RMI_VDEV_GET_STATE,
    .label = VDEV_DEV_MEM_MMIO,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "vdev_gran_state",
    .abi = RMI_VDEV_GET_STATE,
    .label = VDEV_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
