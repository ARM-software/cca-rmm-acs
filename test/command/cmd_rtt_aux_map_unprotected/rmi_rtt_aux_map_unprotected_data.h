/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
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
    IPA_UNALIGNED = 0X8,
    IPA_PROTECTED = 0X9,
    IPA_OOB = 0XA,
    REALM_SINGLE_RTT_TREE = 0XB,
    INDEX_PRIMARY_RTT_TREE = 0XC,
    INDEX_OUT_OF_BOUND = 0XD,
    PRI_STATE_UNASSIGNED_NS = 0XE,
    AUX_RTT_UNMAPPED = 0XF,
    INDEX_OOB_PRI_STATE_UNASSINGED_NS = 0X10
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
    .abi = RMI_RTT_AUX_MAP_UNPROTECTED,
    .label = RD_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_bound",
    .abi = RMI_RTT_AUX_MAP_UNPROTECTED,
    .label = RD_DEV_MEM,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_bound",
    .abi = RMI_RTT_AUX_MAP_UNPROTECTED,
    .label = RD_OUTSIDE_OF_PERMITTED_PA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_AUX_MAP_UNPROTECTED,
    .label = RD_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_AUX_MAP_UNPROTECTED,
    .label = RD_STATE_DELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_AUX_MAP_UNPROTECTED,
    .label = RD_STATE_REC,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_AUX_MAP_UNPROTECTED,
    .label = RD_STATE_RTT,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_RTT_AUX_MAP_UNPROTECTED,
    .label = RD_STATE_DATA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "ipa_align",
    .abi = RMI_RTT_AUX_MAP_UNPROTECTED,
    .label = IPA_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "ipa_bound",
    .abi = RMI_RTT_AUX_MAP_UNPROTECTED,
    .label = IPA_PROTECTED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "ipa_bound",
    .abi = RMI_RTT_AUX_MAP_UNPROTECTED,
    .label = IPA_OOB,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "index_bound",
    .abi = RMI_RTT_AUX_MAP_UNPROTECTED,
    .label = REALM_SINGLE_RTT_TREE,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "index_bound",
    .abi = RMI_RTT_AUX_MAP_UNPROTECTED,
    .label = INDEX_PRIMARY_RTT_TREE,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "index_bound",
    .abi = RMI_RTT_AUX_MAP_UNPROTECTED,
    .label = INDEX_OUT_OF_BOUND,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pri_state",
    .abi = RMI_RTT_AUX_MAP_UNPROTECTED,
    .label = PRI_STATE_UNASSIGNED_NS,
    .status = RMI_ERROR_RTT,
    .index = 3},
    {.msg = "level",
    .abi = RMI_RTT_AUX_MAP_UNPROTECTED,
    .label = AUX_RTT_UNMAPPED,
    .status = RMI_ERROR_RTT_AUX,
    .index = 2},
    {.msg = "index_bound_compare_pri_state",
    .abi = RMI_RTT_AUX_MAP_UNPROTECTED,
    .label = INDEX_OOB_PRI_STATE_UNASSINGED_NS,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
