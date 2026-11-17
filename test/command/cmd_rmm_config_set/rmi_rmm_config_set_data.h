/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    RMM_ACTIVE = 0x0
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "rmm_state",
    .abi = RMI_RMM_CONFIG_SET,
    .label = RMM_ACTIVE,
    .status = RMI_ERROR_GLOBAL,
    .index = 0}
};
