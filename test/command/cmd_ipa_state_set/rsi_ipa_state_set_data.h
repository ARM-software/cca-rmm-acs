/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    ADDR_UNALIGNED = 0X0,
    SIZE_UNALLIGNED = 0X1,
    REGION_UNPROTECTED = 0X2,
    RIPAS_INVALID = 0X3
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
    .abi = RSI_IPA_STATE_SET,
    .label = ADDR_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "size_bound",
    .abi = RSI_IPA_STATE_SET,
    .label = SIZE_UNALLIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rgn_bound",
    .abi = RSI_IPA_STATE_SET,
    .label = REGION_UNPROTECTED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "ripas_valid",
    .abi = RSI_IPA_STATE_SET,
    .label = RIPAS_INVALID,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
