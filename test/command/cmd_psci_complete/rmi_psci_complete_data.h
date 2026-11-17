/*
 * Copyright (c) 2023, 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    CALLING_UNALIGNED = 0X0,
    CALLING_GRAN_STATE_UNDELEGATED = 0X1,
    CALLING_GRAN_STATE_DELEGATED = 0X2,
    CALLING_GRAN_STATE_RD = 0X3,
    CALLING_GRAN_STATE_RTT = 0X4,
    CALLING_GRAN_STATE_DATA = 0X5,
    NO_PSCI_REQUEST = 0X6,
    STATUS_NOT_PERMITTED = 0X7,
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "calling_align",
    .abi = RMI_PSCI_COMPLETE,
    .label = CALLING_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "calling state",
    .abi = RMI_PSCI_COMPLETE,
    .label = CALLING_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "calling state",
    .abi = RMI_PSCI_COMPLETE,
    .label = CALLING_GRAN_STATE_DELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "calling state",
    .abi = RMI_PSCI_COMPLETE,
    .label = CALLING_GRAN_STATE_RD,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "calling state",
    .abi = RMI_PSCI_COMPLETE,
    .label = CALLING_GRAN_STATE_RTT,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "calling state",
    .abi = RMI_PSCI_COMPLETE,
    .label = CALLING_GRAN_STATE_DATA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pending",
    .abi = RMI_PSCI_COMPLETE,
    .label = NO_PSCI_REQUEST,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "status",
    .abi = RMI_PSCI_COMPLETE,
    .label = STATUS_NOT_PERMITTED,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
