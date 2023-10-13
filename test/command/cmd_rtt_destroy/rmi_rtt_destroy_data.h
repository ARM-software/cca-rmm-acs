/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    RD_UNALIGNED = 0X0,
    RD_OUTSIDE_OF_PERMITTED_PA = 0X1,
    RD_DEV_MEM = 0X2,
    RD_STATE_UNDELEGATED = 0X3,
    RD_STATE_DELEGATED = 0X4,
    RD_STATE_REC = 0X5,
    RD_STATE_RTT = 0X6,
    RD_STATE_DATA = 0X7,
    LEVEL_INVALID = 0X8,
    LEVEL_STARTING_LEVEL = 0X9,
    IPA_LEVEL_UNALIGNED = 0XA,
    IPA_OUT_OF_PERMITTED_IPA = 0XB,
    IPA_NO_PARENT_RTTE = 0XC,
    RTTE_STATE_UNASSIGNED = 0XD,
    RTTE_STATE_ASSIGNED = 0XE,
    RTT_LIVE = 0XF
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
    .abi = RMI_RTT_DESTROY,
    .label = RD_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_bound",
    .abi = RMI_RTT_DESTROY,
    .label = RD_OUTSIDE_OF_PERMITTED_PA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_bound",
    .abi = RMI_RTT_DESTROY,
    .label = RD_DEV_MEM,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_DESTROY,
    .label = RD_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_DESTROY,
    .label = RD_STATE_DELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_DESTROY,
    .label = RD_STATE_REC,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_DESTROY,
    .label = RD_STATE_RTT,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_DESTROY,
    .label = RD_STATE_DATA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "level_bound",
    .abi = RMI_RTT_DESTROY,
    .label = LEVEL_INVALID,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "level_bound",
    .abi = RMI_RTT_DESTROY,
    .label = LEVEL_STARTING_LEVEL,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "ipa_align",
    .abi = RMI_RTT_DESTROY,
    .label = IPA_LEVEL_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "ipa_bound",
    .abi = RMI_RTT_DESTROY,
    .label = IPA_OUT_OF_PERMITTED_IPA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rtt_walk",
    .abi = RMI_RTT_DESTROY,
    .label = IPA_NO_PARENT_RTTE,
    .status = RMI_ERROR_RTT,
    .index = 1},
    {.msg = "rtte_state",
    .abi = RMI_RTT_DESTROY,
    .label = RTTE_STATE_UNASSIGNED,
    .status = RMI_ERROR_RTT,
    .index = 2},
    {.msg = "rtte_state",
    .abi = RMI_RTT_DESTROY,
    .label = RTTE_STATE_ASSIGNED,
    .status = RMI_ERROR_RTT,
    .index = 2},
    {.msg = "rtt_live",
    .abi = RMI_RTT_DESTROY,
    .label = RTT_LIVE,
    .status = RMI_ERROR_RTT,
    .index = 3}
};
