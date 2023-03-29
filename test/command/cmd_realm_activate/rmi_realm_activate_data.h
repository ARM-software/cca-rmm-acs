/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    RD_UNALIGNED = 0X0,
    RD_DEV_MEM_MMIO = 0X1,
    RD_OUTSIDE_OF_PERMITTED_PA = 0X2,
    RD_STATE_UNDELEGATED = 0X3,
    RD_STATE_DELEGATED = 0X4,
    RD_REC = 0X5,
    RD_DATA = 0X6,
    RD_RTT = 0X7,
    REALM_SYSTEM_OFF = 0X8,
    REALM_NULL = 0X9
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "rd_align",
    .abi = RMI_REALM_ACTIVATE,
    .label = RD_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_bound",
    .abi = RMI_REALM_ACTIVATE,
    .label = RD_DEV_MEM_MMIO,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_bound",
    .abi = RMI_REALM_ACTIVATE,
    .label = RD_OUTSIDE_OF_PERMITTED_PA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_REALM_ACTIVATE,
    .label = RD_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_REALM_ACTIVATE,
    .label = RD_STATE_DELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_REALM_ACTIVATE,
    .label = RD_REC,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_REALM_ACTIVATE,
    .label = RD_DATA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_REALM_ACTIVATE,
    .label = RD_RTT,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "realm_state",
    .abi = RMI_REALM_ACTIVATE,
    .label = REALM_SYSTEM_OFF,
    .status = RMI_ERROR_REALM,
    .index = 0},
    {.msg = "realm_state_null",
    .abi = RMI_REALM_ACTIVATE,
    .label = REALM_NULL,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
