/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    HANDLE_INVALID = 0X0,
    HANDLE_CANNOT_CANCEL = 0X1
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "complete",
    .abi = RMI_OP_CANCEL,
    .label = HANDLE_INVALID,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "can_cancel",
    .abi = RMI_OP_CANCEL,
    .label = HANDLE_CANNOT_CANCEL,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
