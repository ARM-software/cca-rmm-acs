/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    ADDR_UNALIGNED = 0X0,
    ADDR_UNPROTECTED = 0X1
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "addr_align",
    .abi = RSI_REALM_CONFIG,
    .label = ADDR_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "addr_bound",
    .abi = RSI_REALM_CONFIG,
    .label = ADDR_UNPROTECTED,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
