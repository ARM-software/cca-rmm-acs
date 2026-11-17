/*
 * Copyright (c) 2023-2024, 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    RD_UNALIGNED = 0X0,
    RD_STATE_UNDELEGATED,
    RD_STATE_DELEGATED,
    RD_STATE_REC,
    RD_STATE_RTT,
    RD_STATE_DATA,
    BASE_UNALIGNED,
    TOP_UNALIGNED,
    SIZE_INVALID,
    IPA_PROTECTED,
    OADDR_TYPE_NONE,
    OADDR_TYPE_RESERVED,
    OADDR_LIST_UNALIGNED,
    OADDR_LIST_INVALID_PAS,
    RTTE_STATE_ASSIGNED_NS,
    RTTE_SIZE,
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
    .abi = RMI_RTT_UNPROT_MAP,
    .label = RD_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_UNPROT_MAP,
    .label = RD_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_UNPROT_MAP,
    .label = RD_STATE_DELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_UNPROT_MAP,
    .label = RD_STATE_REC,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_UNPROT_MAP,
    .label = RD_STATE_RTT,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_UNPROT_MAP,
    .label = RD_STATE_DATA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "base_align",
    .abi = RMI_RTT_UNPROT_MAP,
    .label = BASE_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "top_align",
    .abi = RMI_RTT_UNPROT_MAP,
    .label = TOP_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "size_valid",
    .abi = RMI_RTT_UNPROT_MAP,
    .label = SIZE_INVALID,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "ipa_bound",
    .abi = RMI_RTT_UNPROT_MAP,
    .label = IPA_PROTECTED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "oaddr_type",
    .abi = RMI_RTT_UNPROT_MAP,
    .label = OADDR_TYPE_NONE,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "oaddr_type",
    .abi = RMI_RTT_UNPROT_MAP,
    .label = OADDR_TYPE_RESERVED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "oaddr_align",
    .abi = RMI_RTT_UNPROT_MAP,
    .label = OADDR_LIST_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "oaddr_list_pas",
    .abi = RMI_RTT_UNPROT_MAP,
    .label = OADDR_LIST_INVALID_PAS,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rtte_state",
    .abi = RMI_RTT_UNPROT_MAP,
    .label = RTTE_STATE_ASSIGNED_NS,
    .status = RMI_ERROR_RTT,
    .index = 3},
    {.msg = "rtte_size",
    .abi = RMI_RTT_UNPROT_MAP,
    .label = RTTE_SIZE,
    .status = RMI_ERROR_RTT,
    .index = 2}
};
