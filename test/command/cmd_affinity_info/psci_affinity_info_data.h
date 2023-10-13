/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_psci.h"

enum test_intent {
    INVALID_LOWEST_AFFINITY_LEVEL = 0x0,
    MPIDR_NOT_USED = 0x1,
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
};

static struct stimulus test_data[] = {
    {.msg = "target_bound",
    .abi = PSCI_AFFINITY_INFO_AARCH64,
    .label = INVALID_LOWEST_AFFINITY_LEVEL,
    .status = PSCI_E_INVALID_PARAMS},
    {.msg = "target_match",
    .abi = PSCI_AFFINITY_INFO_AARCH64,
    .label = MPIDR_NOT_USED,
    .status = PSCI_E_INVALID_PARAMS}
};
