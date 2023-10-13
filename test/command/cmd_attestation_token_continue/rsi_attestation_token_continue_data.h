/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    ADDR = 0X0,
    STATE = 0X1
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
};

static struct stimulus test_data[] = {
    {.msg = "addr",
    .abi = RSI_MEASUREMENT_EXTEND,
    .label = ADDR,
    .status = RSI_ERROR_INPUT},
    {.msg = "state",
    .abi = RSI_MEASUREMENT_EXTEND,
    .label = STATE,
    .status = RSI_ERROR_STATE}
};