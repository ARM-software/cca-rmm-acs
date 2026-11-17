/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    feat = 0x0,
    l0dpt_state
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "feat",
    .abi = RMI_DPT_L0_DESTROY,
    .label = feat,
    .status = RMI_ERROR_NOT_SUPPORTED,
    .index = 0},
    {.msg = "l0dpt_state",
    .abi = RMI_DPT_L0_DESTROY,
    .label = l0dpt_state,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
