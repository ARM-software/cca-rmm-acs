/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    l0dpt_state = 0x0,
    addr_bound,
    addr_align,
    entry_state
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "l0dpt_state",
    .abi = RMI_DPT_L1_CREATE,
    .label = l0dpt_state,
    .status = RMI_ERROR_GLOBAL,
    .index = 0},
    {.msg = "addr_bound",
    .abi = RMI_DPT_L1_CREATE,
    .label = addr_bound,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "addr_align",
    .abi = RMI_DPT_L1_CREATE,
    .label = addr_align,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "entry_state",
    .abi = RMI_DPT_L1_CREATE,
    .label = entry_state,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
