/*
 * Copyright (c) 2023, 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"
#include "val_realm_rsi_types.h"

enum test_intent {
    ADDR_ALIGN = 0X0,
    ADDR_BOUND = 0X1,
    ADDR_EMPTY = 0X2,
    OFFSET_BOUND = 0X3,
    SIZE_OVERFLOW = 0X4,
    SIZE_BOUND = 0X5,
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
};

static struct stimulus test_data[] = {
    {.msg = "addr_align",
    .abi = RSI_ATTESTATION_TOKEN_CONTINUE,
    .label = ADDR_ALIGN,
    .status = RSI_ERROR_INPUT},
    {.msg = "addr_bound",
    .abi = RSI_ATTESTATION_TOKEN_CONTINUE,
    .label = ADDR_BOUND,
    .status = RSI_ERROR_INPUT},
    {.msg = "addr_empty",
    .abi = RSI_ATTESTATION_TOKEN_CONTINUE,
    .label = ADDR_EMPTY,
    .status = RSI_ERROR_INPUT},
    {.msg = "offset_bound",
    .abi = RSI_ATTESTATION_TOKEN_CONTINUE,
    .label = OFFSET_BOUND,
    .status = RSI_ERROR_INPUT},
    {.msg = "size_overflow",
    .abi = RSI_ATTESTATION_TOKEN_CONTINUE,
    .label = SIZE_OVERFLOW,
    .status = RSI_ERROR_INPUT},
    {.msg = "size_bound",
    .abi = RSI_ATTESTATION_TOKEN_CONTINUE,
    .label = SIZE_BOUND,
    .status = RSI_ERROR_INPUT}
};
