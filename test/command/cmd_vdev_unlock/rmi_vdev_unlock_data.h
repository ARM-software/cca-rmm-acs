/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    RD_UNALIGNED = 0X0,
    RD_GRAN_STATE_UNDELEGATED = 0X1,
    PDEV_UNALIGNED = 0X2,
    PDEV_GRAN_STATE_UNDELEGATED = 0X3,
    VDEV_UNALIGNED = 0X4,
    VDEV_GRAN_STATE_UNDELEGATED = 0X5,
    INVALID_RD = 0X6,
    INVALID_PDEV = 0X7,
    VDEV_STATE_VDEV_UNLOCKED = 0X8,
    VDEV_GRAN_STATE_UNDELEGATED_INVALID_PDEV = 0X9,
    GRAN_MAPPED = 0XA
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "rd_align",
    .abi = RMI_VDEV_UNLOCK,
    .label = RD_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_VDEV_UNLOCK,
    .label = RD_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pdev_align",
    .abi = RMI_VDEV_UNLOCK,
    .label = PDEV_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pdev_gran_state",
    .abi = RMI_VDEV_UNLOCK,
    .label = PDEV_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "vdev_align",
    .abi = RMI_VDEV_UNLOCK,
    .label = VDEV_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "vdev_gran_state",
    .abi = RMI_VDEV_UNLOCK,
    .label = VDEV_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "vdev_realm",
    .abi = RMI_VDEV_UNLOCK,
    .label = INVALID_RD,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "vdev_pdev",
    .abi = RMI_VDEV_UNLOCK,
    .label = INVALID_PDEV,
    .status = RMI_ERROR_DEVICE,
    .index = 0},
    {.msg = "vdev_state",
    .abi = RMI_VDEV_UNLOCK,
    .label = VDEV_STATE_VDEV_UNLOCKED,
    .status = RMI_ERROR_DEVICE,
    .index = 0},
    {.msg = "vdev_gran_state_vdev_pdev",
    .abi = RMI_VDEV_UNLOCK,
    .label = VDEV_GRAN_STATE_UNDELEGATED_INVALID_PDEV,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "gran_mapped",
    .abi = RMI_VDEV_UNLOCK,
    .label = GRAN_MAPPED,
    .status = RMI_ERROR_GRANULE,
    .index = 0}
};
