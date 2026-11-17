/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    INCOMPAT_LOWER = 0x0,
    INCOMPAT_HIGHER = 0x1
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "incompat_lower",
    .abi = RMI_VERSION,
    .label = INCOMPAT_LOWER,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "incompat_higher",
    .abi = RMI_VERSION,
    .label = INCOMPAT_HIGHER,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
