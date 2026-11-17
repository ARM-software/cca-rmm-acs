/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    psmmu_valid = 0x0,
    psmmu_state,
    psmmu_live
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "psmmu_valid",
    .abi = RMI_PSMMU_DEACTIVATE,
    .label = psmmu_valid,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "psmmu_state",
    .abi = RMI_PSMMU_DEACTIVATE,
    .label = psmmu_state,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "psmmu_live",
    .abi = RMI_PSMMU_DEACTIVATE,
    .label = psmmu_live,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
