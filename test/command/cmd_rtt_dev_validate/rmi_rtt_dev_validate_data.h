/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    RD_UNALIGNED = 0X0,
    RD_GRAN_STATE_UNDELEGATED = 0X1,
    REC_UNALIGNED = 0X2,
    REC_GRAN_STATE_UNDELEGATED = 0X3,
    REC_OWNER = 0X4,
    SIZE_INVALID = 0X5,
    BASE_BOUND = 0X6,
    TOP_BOUND = 0X7,
    TOP_GRAN_ALIGN = 0X8,
    BASE_LEVEL_UNALIGNED = 0X9,
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
    .abi = RMI_RTT_DEV_VALIDATE,
    .label = RD_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_DEV_VALIDATE,
    .label = RD_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_align",
    .abi = RMI_RTT_DEV_VALIDATE,
    .label = REC_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_gran_state",
    .abi = RMI_RTT_DEV_VALIDATE,
    .label = REC_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_owner",
    .abi = RMI_RTT_DEV_VALIDATE,
    .label = REC_OWNER,
    .status = RMI_ERROR_REC,
    .index = 0},
    {.msg = "size_valid",
    .abi = RMI_RTT_DEV_VALIDATE,
    .label = SIZE_INVALID,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "base_bound",
    .abi = RMI_RTT_DEV_VALIDATE,
    .label = BASE_BOUND,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "top_bound",
    .abi = RMI_RTT_DEV_VALIDATE,
    .label = TOP_BOUND,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "top_align",
    .abi = RMI_RTT_DEV_VALIDATE,
    .label = TOP_GRAN_ALIGN,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "base_align",
    .abi = RMI_RTT_DEV_VALIDATE,
    .label = BASE_LEVEL_UNALIGNED,
    .status = RMI_ERROR_RTT,
    .index = 0},
};
