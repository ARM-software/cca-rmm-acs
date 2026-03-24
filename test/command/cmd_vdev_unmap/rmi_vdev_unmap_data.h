/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    RD_UNALIGNED = 0X0,
    RD_OUTSIDE_OF_PERMITTED_PA = 0X1,
    RD_DEV_MEM_MMIO = 0X2,
    RD_GRAN_STATE_UNDELEGATED = 0X3,
    VDEV_UNALIGNED = 0X4,
    VDEV_OUTSIDE_OF_PERMITTED_PA = 0X5,
    VDEV_DEV_MEM_MMIO = 0X6,
    VDEV_GRAN_STATE_UNDELEGATED = 0X7,
    VDEV_REALM = 0X8,
    LEVEL_LT_2 = 0X9,
    LEVEL_OOB = 0XA,
    IPA_UNALIGNED = 0XB,
    IPA_UNPROTECTED = 0XC,
    RTT_WALK = 0XD,
    RTTE_STATE = 0XE
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
    .abi = RMI_VDEV_UNMAP,
    .label = RD_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_bound",
    .abi = RMI_VDEV_UNMAP,
    .label = RD_OUTSIDE_OF_PERMITTED_PA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_bound",
    .abi = RMI_VDEV_UNMAP,
    .label = RD_DEV_MEM_MMIO,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_VDEV_UNMAP,
    .label = RD_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    /* TODO: Uncomment once TF-RMM fix the issue */
    // {.msg = "vdev_align",
    // .abi = RMI_VDEV_UNMAP,
    // .label = VDEV_UNALIGNED,
    // .status = RMI_ERROR_INPUT,
    // .index = 0},
    // {.msg = "vdev_bound",
    // .abi = RMI_VDEV_UNMAP,
    // .label = VDEV_OUTSIDE_OF_PERMITTED_PA,
    // .status = RMI_ERROR_INPUT,
    // .index = 0},
    // {.msg = "vdev_bound",
    // .abi = RMI_VDEV_UNMAP,
    // .label = VDEV_DEV_MEM_MMIO,
    // .status = RMI_ERROR_INPUT,
    // .index = 0},
    // {.msg = "vdev_gran_state",
    // .abi = RMI_VDEV_UNMAP,
    // .label = VDEV_GRAN_STATE_UNDELEGATED,
    // .status = RMI_ERROR_INPUT,
    // .index = 0},
    // {.msg = "vdev_realm",
    // .abi = RMI_VDEV_UNMAP,
    // .label = VDEV_REALM,
    // .status = RMI_ERROR_INPUT,
    // .index = 0},
    {.msg = "level_bound",
    .abi = RMI_VDEV_UNMAP,
    .label = LEVEL_LT_2,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "level_bound",
    .abi = RMI_VDEV_UNMAP,
    .label = LEVEL_OOB,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "ipa_align",
    .abi = RMI_VDEV_UNMAP,
    .label = IPA_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "ipa_bound",
    .abi = RMI_VDEV_UNMAP,
    .label = IPA_UNPROTECTED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rtt_walk",
    .abi = RMI_VDEV_UNMAP,
    .label = RTT_WALK,
    .status = RMI_ERROR_RTT,
    .index = 2},
    {.msg = "rtte_state",
    .abi = RMI_VDEV_UNMAP,
    .label = RTTE_STATE,
    .status = RMI_ERROR_RTT,
    .index = 3}
};
