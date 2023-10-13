/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_psci.h"

enum test_intent {
    ENTRY_ADDR_UNPROTECTED = 0x0,
    MPIDR_NOT_USED = 0x1,
    REC_RUNNABLE = 0x2
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
};

static struct stimulus test_data[] = {
    {.msg = "entry",
    .abi = PSCI_CPU_ON_AARCH64,
    .label = ENTRY_ADDR_UNPROTECTED,
    .status = PSCI_E_INVALID_ADDRESS},
    {.msg = "mpidr",
    .abi = PSCI_CPU_ON_AARCH64,
    .label = MPIDR_NOT_USED,
    .status = PSCI_E_INVALID_PARAMS},
    {.msg = "runnable",
    .abi = PSCI_CPU_ON_AARCH64,
    .label = REC_RUNNABLE,
    .status = PSCI_E_ALREADY_ON}
};
