/*
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    MECID_BOUND = 0X0,
    MEC_STATE_PRIVATE = 0X1
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "mecid_bound",
    .abi = RMI_MEC_SET_SHARED,
    .label = MECID_BOUND,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "state",
    .abi = RMI_MEC_SET_SHARED,
    .label = MEC_STATE_PRIVATE,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
