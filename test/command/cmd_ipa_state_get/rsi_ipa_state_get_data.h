/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    BASE_UNALIGNED = 0X0,
    TOP_UNALIGNED = 0X1,
    SIZE_INVALID = 0X2,
    REGION_UNPROTECTED = 0X3
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
    .abi = RSI_IPA_STATE_GET,
    .label = BASE_UNALIGNED,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "end_align",
    .abi = RSI_IPA_STATE_GET,
    .label = TOP_UNALIGNED,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "size_valid",
    .abi = RSI_IPA_STATE_GET,
    .label = SIZE_INVALID,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "rgn_bound",
    .abi = RSI_IPA_STATE_GET,
    .label = REGION_UNPROTECTED,
    .status = RSI_ERROR_INPUT,
    .index = 0}
};
