/*
 * Copyright (c) 2023, 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"
#include "val_realm_rsi_types.h"

enum test_intent {
    ADDR_UNALIGNED = 0X0,
    ADDR_UNPROTECTED = 0X1,
    ADDR_EMPTY = 0X2
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
};

static struct stimulus test_data[] = {
    {.msg = "addr_align",
    .abi = RSI_REALM_CONFIG,
    .label = ADDR_UNALIGNED,
    .status = RSI_ERROR_INPUT},
    {.msg = "addr_bound",
    .abi = RSI_REALM_CONFIG,
    .label = ADDR_UNPROTECTED,
    .status = RSI_ERROR_INPUT},
    {.msg = "addr_empty",
    .abi = RSI_REALM_CONFIG,
    .label = ADDR_EMPTY,
    .status = RSI_ERROR_INPUT}
};
