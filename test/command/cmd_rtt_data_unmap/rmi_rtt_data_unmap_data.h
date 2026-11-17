/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
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
    BASE_UNALIGNED = 0X6,
    TOP_UNALIGNED = 0X7,
    SIZE_INVALID = 0X8,
    IPA_UNPROTECTED = 0X9,
    OADDR_LIST_UNALIGNED = 0XA,
    OADDR_LIST_INVALID_PAS = 0XB,
    RTTE_SIZE = 0XC,
    TRACKING_COARSE = 0XD,
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
    .abi = RMI_RTT_DATA_UNMAP,
    .label = RD_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_DATA_UNMAP,
    .label = RD_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_DATA_UNMAP,
    .label = RD_STATE_DELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_DATA_UNMAP,
    .label = RD_STATE_REC,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_DATA_UNMAP,
    .label = RD_STATE_RTT,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_DATA_UNMAP,
    .label = RD_STATE_DATA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "base_align",
    .abi = RMI_RTT_DATA_UNMAP,
    .label = BASE_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "top_align",
    .abi = RMI_RTT_DATA_UNMAP,
    .label = TOP_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "size_valid",
    .abi = RMI_RTT_DATA_UNMAP,
    .label = SIZE_INVALID,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "ipa_bound",
    .abi = RMI_RTT_DATA_UNMAP,
    .label = IPA_UNPROTECTED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "oaddr_align",
    .abi = RMI_RTT_DATA_UNMAP,
    .label = OADDR_LIST_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "oaddr_list_pas",
    .abi = RMI_RTT_DATA_UNMAP,
    .label = OADDR_LIST_INVALID_PAS,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rtte_size",
    .abi = RMI_RTT_DATA_UNMAP,
    .label = RTTE_SIZE,
    .status = RMI_ERROR_RTT,
    .index = 2},
    {.msg = "tracking",
    .abi = RMI_RTT_DATA_UNMAP,
    .label = TRACKING_COARSE,
    .status = RMI_ERROR_TRACKING,
    .index = 0},
};
