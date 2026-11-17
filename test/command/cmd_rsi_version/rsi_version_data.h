/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_realm_rsi.h"

enum test_intent {
    INCOMPAT_LOWER = 0x0,
    INCOMPAT_HIGHER = 0x1
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
};

static struct stimulus test_data[] = {
    {.msg = "incompat_lower",
    .abi = RSI_VERSION,
    .label = INCOMPAT_LOWER,
    .status = RSI_ERROR_INPUT},
    {.msg = "incompat_higher",
    .abi = RSI_VERSION,
    .label = INCOMPAT_HIGHER,
    .status = RSI_ERROR_INPUT}
};
