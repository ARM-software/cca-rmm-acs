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
    PERM_INDEX_OUT_OF_BOUND = 0X2,
    PERM_INDEX_LOCKED = 0X3,
    PERM_VALUE_UNSUPPORTED = 0X4
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
    .abi = RSI_MEM_SET_PERM_VALUE,
    .label = PLANE_INDEX_0,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "plane_bound",
    .abi = RSI_MEM_SET_PERM_VALUE,
    .label = PLANE_INDEX_OUT_OF_BOUND,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "perm_bound",
    .abi = RSI_MEM_SET_PERM_VALUE,
    .label = PERM_INDEX_OUT_OF_BOUND,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "locked",
    .abi = RSI_MEM_SET_PERM_VALUE,
    .label = PERM_INDEX_LOCKED,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "supported",
    .abi = RSI_MEM_SET_PERM_VALUE,
    .label = PERM_VALUE_UNSUPPORTED,
    .status = RSI_ERROR_INPUT,
    .index = 0}
};
