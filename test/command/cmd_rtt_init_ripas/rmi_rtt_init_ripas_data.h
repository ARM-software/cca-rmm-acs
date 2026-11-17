/*
 * Copyright (c) 2024, 2026, Arm Limited or its affiliates. All rights reserved.
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
    SIZE_INVALID = 0X6,
    TOP_UNPROTECTED = 0X7,
    REALM_SYSTEM_OFF = 0X8,
    REALM_ACTIVE = 0X9,
    REALM_NULL = 0XA,
    BASE_LEVEL_UNALIGNED = 0XB,
    TOP_GRAN_UNALIGNED = 0XC,
    NO_PROGRESS_SHORT_RANGE = 0XD,
    TOP_GRAN_UNALIGNED_NO_PROGRESS = 0XE,
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
    .label = REALM_SYSTEM_OFF,
    .status = RMI_ERROR_REALM,
    .index = 0},
    {.msg = "realm_state",
    .abi = RMI_RTT_INIT_RIPAS,
    .label = REALM_ACTIVE,
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
    {.msg = "top_gran_align",
    .abi = RMI_RTT_INIT_RIPAS,
    .label = TOP_GRAN_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "no_progress",
    .abi = RMI_RTT_INIT_RIPAS,
    .label = NO_PROGRESS_SHORT_RANGE,
    .status = RMI_ERROR_RTT,
    .index = 2},
    {.msg = "top_gran_align_compare_no_progress",
    .abi = RMI_RTT_INIT_RIPAS,
    .label = TOP_GRAN_UNALIGNED_NO_PROGRESS,
    .status = RMI_ERROR_INPUT,
    .index = 0},
};
