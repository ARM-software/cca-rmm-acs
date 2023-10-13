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
    RD_STATE_REC = 0X5,
    RD_STATE_RTT = 0X6,
    RD_STATE_DATA = 0X7,
    IPA_UNALIGNED = 0X8,
    IPA_UNPROTECTED = 0X9,
    IPA_OUTSIDE_OF_PERMITTED_IPA = 0XA,
    IPA_NOT_MAPPED = 0XB,
    RTTE_STATE_UNASSIGNED = 0XC,
    IPA_UNPROTECTED_NOT_MAPPED = 0XD,
    IPA_UNPROTECTED_RTTE_UNASSIGNED = 0XE
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
    .abi = RMI_DATA_DESTROY,
    .label = RD_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_bound",
    .abi = RMI_DATA_DESTROY,
    .label = RD_DEV_MEM_MMIO,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_bound",
    .abi = RMI_DATA_DESTROY,
    .label = RD_OUTSIDE_OF_PERMITTED_PA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_DATA_DESTROY,
    .label = RD_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_DATA_DESTROY,
    .label = RD_STATE_DELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_DATA_DESTROY,
    .label = RD_STATE_REC,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_DATA_DESTROY,
    .label = RD_STATE_RTT,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_DATA_DESTROY,
    .label = RD_STATE_DATA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "ipa_align",
    .abi = RMI_DATA_DESTROY,
    .label = IPA_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "ipa_bound",
    .abi = RMI_DATA_DESTROY,
    .label = IPA_UNPROTECTED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "ipa_bound",
    .abi = RMI_DATA_DESTROY,
    .label = IPA_OUTSIDE_OF_PERMITTED_IPA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rtt_walk",
    .abi = RMI_DATA_DESTROY,
    .label = IPA_NOT_MAPPED,
    .status = RMI_ERROR_RTT,
    .index = 2},
    {.msg = "rtte_state",
    .abi = RMI_DATA_DESTROY,
    .label = RTTE_STATE_UNASSIGNED,
    .status = RMI_ERROR_RTT,
    .index = 3},
    {.msg = "ipa_bound_compare_rtt_walk",
    .abi = RMI_DATA_DESTROY,
    .label = IPA_UNPROTECTED_NOT_MAPPED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "ipa_bound_compare_rtte_state",
    .abi = RMI_DATA_DESTROY,
    .label = IPA_UNPROTECTED_RTTE_UNASSIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
