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
    BASE_UNALIGNED = 0X2,
    TOP_UNALIGNED = 0X3,
    SIZE_INVALID = 0X4,
    IPA_UNPROTECTED = 0X5,
    RTT_WALK = 0X6,
    RTTE_STATE = 0X7,
    OADDR_LIST_UNALIGNED = 0X8,
    OADDR_LIST_INVALID_PAS = 0X9
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
    .abi = RMI_RTT_DEV_UNMAP,
    .label = RD_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_DEV_UNMAP,
    .label = RD_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "base_align",
    .abi = RMI_RTT_DEV_UNMAP,
    .label = BASE_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "top_align",
    .abi = RMI_RTT_DEV_UNMAP,
    .label = TOP_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "size_valid",
    .abi = RMI_RTT_DEV_UNMAP,
    .label = SIZE_INVALID,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "ipa_bound",
    .abi = RMI_RTT_DEV_UNMAP,
    .label = IPA_UNPROTECTED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rtte_state",
    .abi = RMI_RTT_DEV_UNMAP,
    .label = RTT_WALK,
    .status = RMI_ERROR_RTT,
    .index = 2},
    {.msg = "rtte_state",
    .abi = RMI_RTT_DEV_UNMAP,
    .label = RTTE_STATE,
    .status = RMI_ERROR_RTT,
    .index = 3},
    {.msg = "oaddr_align",
    .abi = RMI_RTT_DEV_UNMAP,
    .label = OADDR_LIST_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "oaddr_list_pas",
    .abi = RMI_RTT_DEV_UNMAP,
    .label = OADDR_LIST_INVALID_PAS,
    .status = RMI_ERROR_INPUT,
    .index = 0},
};
