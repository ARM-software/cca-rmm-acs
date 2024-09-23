/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
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
    REALM_SINGLE_RTT_TREE = 0XB,
    INDEX_PRIMARY_RTT_TREE = 0XC,
    INDEX_OUT_OF_BOUND = 0XD,
    RTTE_STATE_UNASSIGNED = 0XE,
    INDEX_OOB_RTTE_STATE_UNASSIGNED = 0XF
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
    .abi = RMI_RTT_AUX_UNMAP_PROTECTED,
    .label = RD_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_bound",
    .abi = RMI_RTT_AUX_UNMAP_PROTECTED,
    .label = RD_DEV_MEM_MMIO,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_bound",
    .abi = RMI_RTT_AUX_UNMAP_PROTECTED,
    .label = RD_OUTSIDE_OF_PERMITTED_PA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_AUX_UNMAP_PROTECTED,
    .label = RD_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_AUX_UNMAP_PROTECTED,
    .label = RD_STATE_DELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_AUX_UNMAP_PROTECTED,
    .label = RD_STATE_REC,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_AUX_UNMAP_PROTECTED,
    .label = RD_STATE_RTT,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_AUX_UNMAP_PROTECTED,
    .label = RD_STATE_DATA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "ipa_align",
    .abi = RMI_RTT_AUX_UNMAP_PROTECTED,
    .label = IPA_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "ipa_bound",
    .abi = RMI_RTT_AUX_UNMAP_PROTECTED,
    .label = IPA_UNPROTECTED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "ipa_bound",
    .abi = RMI_RTT_AUX_UNMAP_PROTECTED,
    .label = IPA_OUTSIDE_OF_PERMITTED_IPA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "index_bound",
    .abi = RMI_RTT_AUX_UNMAP_PROTECTED,
    .label = REALM_SINGLE_RTT_TREE,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "index_bound",
    .abi = RMI_RTT_AUX_UNMAP_PROTECTED,
    .label = INDEX_PRIMARY_RTT_TREE,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "index_bound",
    .abi = RMI_RTT_AUX_UNMAP_PROTECTED,
    .label = INDEX_OUT_OF_BOUND,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rtte_state",
    .abi = RMI_RTT_AUX_UNMAP_PROTECTED,
    .label = RTTE_STATE_UNASSIGNED,
    .status = RMI_ERROR_RTT_AUX,
    .index = 3},
    {.msg = "index_bound_compare_rtte_state",
    .abi = RMI_RTT_AUX_UNMAP_PROTECTED,
    .label = INDEX_OOB_RTTE_STATE_UNASSIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
