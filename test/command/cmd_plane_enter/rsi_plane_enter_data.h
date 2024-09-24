/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    PLANE_INDEX_0 = 0X0,
    PLANE_INDEX_OUT_OF_BOUND = 0X1,
    RUN_PTR_UNALIGNED = 0X2,
    RUN_PTR_UNPROTECTED = 0X3
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "idx_bound",
    .abi = RSI_PLANE_ENTER,
    .label = PLANE_INDEX_0,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "idx_bound",
    .abi = RSI_PLANE_ENTER,
    .label = PLANE_INDEX_OUT_OF_BOUND,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "run_align",
    .abi = RSI_PLANE_ENTER,
    .label = RUN_PTR_UNALIGNED,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "run_bound",
    .abi = RSI_PLANE_ENTER,
    .label = RUN_PTR_UNPROTECTED,
    .status = RSI_ERROR_INPUT,
    .index = 0}
};
