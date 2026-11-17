/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    PDEV_UNALIGNED = 0X0,
    PDEV_TRACK_COARSE = 0X1,
    PDEV_GRAN_STATE_UNDELEGATED = 0X2,
    PARAMS_UNALIGNED = 0X3,
    PARAMS_PAS_REALM = 0X4,
    PARAMS_PAS_SECURE = 0X5,
    PARAMS_INVALID = 0X6,
    HB_BASE_INVALID = 0X7,
    PDEV_ID_INVALID = 0X8,
    FLAGS_UNSUPPORTED = 0X9,
    MAX_NUM_VDEVS = 0XA,
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
    .abi = RMI_PDEV_CREATE,
    .label = PDEV_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pdev_track",
    .abi = RMI_PDEV_CREATE,
    .label = PDEV_TRACK_COARSE,
    .status = RMI_ERROR_TRACKING,
    .index = 0},
    {.msg = "pdev_state",
    .abi = RMI_PDEV_CREATE,
    .label = PDEV_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "params_align",
    .abi = RMI_PDEV_CREATE,
    .label = PARAMS_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "params_pas",
    .abi = RMI_PDEV_CREATE,
    .label = PARAMS_PAS_REALM,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "params_pas",
    .abi = RMI_PDEV_CREATE,
    .label = PARAMS_PAS_SECURE,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "params_valid",
    .abi = RMI_PDEV_CREATE,
    .label = PARAMS_INVALID,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "params_valid",
    .abi = RMI_PDEV_CREATE,
    .label = HB_BASE_INVALID,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "params_valid",
    .abi = RMI_PDEV_CREATE,
    .label = PDEV_ID_INVALID,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "flags_supp",
    .abi = RMI_PDEV_CREATE,
    .label = FLAGS_UNSUPPORTED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "max_num_vdevs",
    .abi = RMI_PDEV_CREATE,
    .label = MAX_NUM_VDEVS,
    .status = RMI_ERROR_INPUT,
    .index = 0},
};
