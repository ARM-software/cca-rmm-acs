/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    INDEX_LOWER_BOUND = 0X0,
    INDEX_UPPER_BOUND = 0X1,
    SIZE_BOUND = 0x2
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
};

static struct stimulus test_data[] = {
    {.msg = "index_bound",
    .abi = RSI_MEASUREMENT_EXTEND,
    .label = INDEX_LOWER_BOUND,
    .status = RSI_ERROR_INPUT},
    {.msg = "index_bound",
    .abi = RSI_MEASUREMENT_EXTEND,
    .label = INDEX_UPPER_BOUND,
    .status = RSI_ERROR_INPUT},
    {.msg = "size_bound",
    .abi = RSI_MEASUREMENT_EXTEND,
    .label = SIZE_BOUND,
    .status = RSI_ERROR_INPUT}
};