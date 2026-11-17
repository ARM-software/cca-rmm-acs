/*
 * Copyright (c) 2023, 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"
#include "val_realm_rsi_types.h"

enum test_intent {
    INDEX_BOUND = 0X0
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
};

static struct stimulus test_data[] = {
    {.msg = "index_bound",
    .abi = RSI_MEASUREMENT_READ,
    .label = INDEX_BOUND,
    .status = RSI_ERROR_INPUT}
};