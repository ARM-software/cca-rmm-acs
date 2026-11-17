/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    ADDR_BOUND = 0X0,
    ADDR_UNALIGNED = 0X1,
    ENTRY_STATE = 0X2,
    UNFOLD_DENIED = 0X3,
    GRAN_INVALID = 0X4
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "addr_bound",
    .abi = RMI_GPT_L1_CREATE,
    .label = ADDR_BOUND,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "addr_align",
    .abi = RMI_GPT_L1_CREATE,
    .label = ADDR_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "entry_state",
    .abi = RMI_GPT_L1_CREATE,
    .label = ENTRY_STATE,
    .status = RMI_ERROR_GPT,
    .index = 0},
    {.msg = "unfold_denied",
    .abi = RMI_GPT_L1_CREATE,
    .label = UNFOLD_DENIED,
    .status = RMI_ERROR_GLOBAL,
    .index = 0},
    {.msg = "gran",
    .abi = RMI_GPT_L1_CREATE,
    .label = GRAN_INVALID,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
