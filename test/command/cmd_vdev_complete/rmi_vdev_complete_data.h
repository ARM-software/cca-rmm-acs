/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    REC_UNALIGNED = 0X0,
    REC_OUTSIDE_OF_PERMITTED_PA = 0X1,
    REC_DEV_MEM_MMIO = 0X2,
    REC_GRAN_STATE_UNDELEGATED = 0X3,
    VDEV_UNALIGNED = 0X4,
    VDEV_OUTSIDE_OF_PERMITTED_PA = 0X5,
    VDEV_DEV_MEM_MMIO = 0X6,
    VDEV_GRAN_STATE_UNDELEGATED = 0X7,
    NOT_REC_PENDING_VDEV_REQUEST = 0X8,
    INVALID_REC_OWNER = 0X9,
    INVALID_VDEV_ID = 0XA
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "rec_align",
    .abi = RMI_VDEV_COMPLETE,
    .label = REC_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_bound",
    .abi = RMI_VDEV_COMPLETE,
    .label = REC_OUTSIDE_OF_PERMITTED_PA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_bound",
    .abi = RMI_VDEV_COMPLETE,
    .label = REC_DEV_MEM_MMIO,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_state",
    .abi = RMI_VDEV_COMPLETE,
    .label = REC_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "vdev_align",
    .abi = RMI_VDEV_COMPLETE,
    .label = VDEV_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "vdev_bound",
    .abi = RMI_VDEV_COMPLETE,
    .label = VDEV_OUTSIDE_OF_PERMITTED_PA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "vdev_bound",
    .abi = RMI_VDEV_COMPLETE,
    .label = VDEV_DEV_MEM_MMIO,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "vdev_gran_state",
    .abi = RMI_VDEV_COMPLETE,
    .label = VDEV_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pending",
    .abi = RMI_VDEV_COMPLETE,
    .label = NOT_REC_PENDING_VDEV_REQUEST,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "owner",
    .abi = RMI_VDEV_COMPLETE,
    .label = INVALID_REC_OWNER,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "vdev_id",
    .abi = RMI_VDEV_COMPLETE,
    .label = INVALID_VDEV_ID,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
