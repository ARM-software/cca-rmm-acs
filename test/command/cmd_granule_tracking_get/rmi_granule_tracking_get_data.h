/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    BASE_UNALIGNED = 0X0,
    TOP_UNALIGNED = 0X1,
    BASE_BOUND = 0X2,
    TOP_BOUND = 0X3,
    BASE_TOP = 0X4
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
    .abi = RMI_GRANULE_TRACKING_GET,
    .label = BASE_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "top_align",
    .abi = RMI_GRANULE_TRACKING_GET,
    .label = TOP_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "base_bound",
    .abi = RMI_GRANULE_TRACKING_GET,
    .label = BASE_BOUND,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "top_bound",
    .abi = RMI_GRANULE_TRACKING_GET,
    .label = TOP_BOUND,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "base_top",
    .abi = RMI_GRANULE_TRACKING_GET,
    .label = BASE_TOP,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
