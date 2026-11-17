/*
 * Copyright (c) 2023, 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    RD_UNALIGNED = 0X0,
    RD_STATE_UNDELEGATED = 0X1,
    RD_STATE_DELEGATED = 0X2,
    RD_STATE_REC = 0X3,
    RD_STATE_RTT = 0X4,
    RD_STATE_DATA = 0X5,
    REALM_NOT_ZOMBIE = 0X6,
    REALM_LIVE = 0X7,
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
    .abi = RMI_REALM_DESTROY,
    .label = RD_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_REALM_DESTROY,
    .label = RD_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_REALM_DESTROY,
    .label = RD_STATE_DELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_REALM_DESTROY,
    .label = RD_STATE_REC,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_REALM_DESTROY,
    .label = RD_STATE_RTT,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_REALM_DESTROY,
    .label = RD_STATE_DATA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "realm_state",
    .abi = RMI_REALM_DESTROY,
    .label = REALM_NOT_ZOMBIE,
    .status = RMI_ERROR_REALM,
    .index = 0},
    {.msg = "realm_live",
    .abi = RMI_REALM_DESTROY,
    .label = REALM_LIVE,
    .status = RMI_ERROR_REALM,
    .index = 0}
};
