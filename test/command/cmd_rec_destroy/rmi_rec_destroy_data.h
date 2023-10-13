/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    REC_UNALIGNED = 0X0,
    REC_OUTSIDE_OF_PERMITTED_PA = 0X1,
    REC_DEV_MEM = 0X2,
    REC_GRAN_STATE_UNDELEGATED = 0X3,
    REC_GRAN_STATE_RD = 0X4,
    REC_GRAN_STATE_REC_AUX = 0X5,
    REC_GRAN_STATE_RTT = 0X6,
    REC_GRAN_STATE_DATA = 0X7
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "rec_align",
    .abi = RMI_REC_DESTROY,
    .label = REC_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_bound",
    .abi = RMI_REC_DESTROY,
    .label = REC_OUTSIDE_OF_PERMITTED_PA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_bound",
    .abi = RMI_REC_DESTROY,
    .label = REC_DEV_MEM,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_gran_state",
    .abi = RMI_REC_DESTROY,
    .label = REC_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_gran_state",
    .abi = RMI_REC_DESTROY,
    .label = REC_GRAN_STATE_RD,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_gran_state",
    .abi = RMI_REC_DESTROY,
    .label = REC_GRAN_STATE_REC_AUX,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_gran_state",
    .abi = RMI_REC_DESTROY,
    .label = REC_GRAN_STATE_RTT,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_gran_state",
    .abi = RMI_REC_DESTROY,
    .label = REC_GRAN_STATE_DATA,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
