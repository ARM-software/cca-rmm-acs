/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    PLANE_INDEX_OUT_OF_BOUND = 0X0,
    REG_ENCODING_INVALID = 0X1
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "plane_bound",
    .abi = RSI_PLANE_REG_READ,
    .label = PLANE_INDEX_OUT_OF_BOUND,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "reg_valid",
    .abi = RSI_PLANE_REG_READ,
    .label = REG_ENCODING_INVALID,
    .status = RSI_ERROR_INPUT,
    .index = 0}
};
