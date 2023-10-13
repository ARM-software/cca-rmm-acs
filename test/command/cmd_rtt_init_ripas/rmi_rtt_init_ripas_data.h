/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    RD_UNALIGNED = 0X0,
    RD_DEV_MEM = 0X1,
    RD_OUTSIDE_OF_PERMITTED_PA = 0X2,
    RD_STATE_UNDELEGATED = 0X3,
    RD_STATE_DELEGATED = 0X4,
    RD_STATE_REC = 0X5,
    RD_STATE_RTT = 0X6,
    RD_STATE_DATA = 0X7,
    SIZE_INVALID = 0X8,
    TOP_UNPROTECTED = 0X9,
    REALM_ACTIVE = 0XA,
    REALM_SYSTEM_OFF = 0XB,
    REALM_NULL = 0XC,
    BASE_LEVEL_UNALIGNED = 0XD,
    RTTE_STATE_ASSIGNED = 0XE,
    TOP_UNALIGNED = 0XF
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
    .abi = RMI_RTT_INIT_RIPAS,
    .label = RD_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_bound",
    .abi = RMI_RTT_INIT_RIPAS,
    .label = RD_DEV_MEM,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_bound",
    .abi = RMI_RTT_INIT_RIPAS,
    .label = RD_OUTSIDE_OF_PERMITTED_PA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_INIT_RIPAS,
    .label = RD_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_INIT_RIPAS,
    .label = RD_STATE_DELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_INIT_RIPAS,
    .label = RD_STATE_REC,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_INIT_RIPAS,
    .label = RD_STATE_RTT,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_INIT_RIPAS,
    .label = RD_STATE_DATA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "size_valid",
    .abi = RMI_RTT_INIT_RIPAS,
    .label = SIZE_INVALID,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "top_bound",
    .abi = RMI_RTT_INIT_RIPAS,
    .label = TOP_UNPROTECTED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "realm_state",
    .abi = RMI_RTT_INIT_RIPAS,
    .label = REALM_ACTIVE,
    .status = RMI_ERROR_REALM,
    .index = 0},
    {.msg = "realm_state",
    .abi = RMI_RTT_INIT_RIPAS,
    .label = REALM_SYSTEM_OFF,
    .status = RMI_ERROR_REALM,
    .index = 0},
    {.msg = "realm_state_null",
    .abi = RMI_RTT_INIT_RIPAS,
    .label = REALM_NULL,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "base_align",
    .abi = RMI_RTT_INIT_RIPAS,
    .label = BASE_LEVEL_UNALIGNED,
    .status = RMI_ERROR_RTT,
    .index = 2},
    {.msg = "rtte_state",
    .abi = RMI_RTT_INIT_RIPAS,
    .label = RTTE_STATE_ASSIGNED,
    .status = RMI_ERROR_RTT,
    .index = 3},
    {.msg = "top_align",
    .abi = RMI_RTT_INIT_RIPAS,
    .label = TOP_UNALIGNED,
    .status = RMI_ERROR_RTT,
    .index = 2}
};
