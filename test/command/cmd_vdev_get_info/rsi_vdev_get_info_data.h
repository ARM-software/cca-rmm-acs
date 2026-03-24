/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    VDEV_IDS_INVALID_VDEV_ID = 0X0,
    ADDR_UNALIGNED = 0X1,
    ADDR_UNPROTECTED = 0X2,
    ADDR_RIPAS_EMPTY = 0X3
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data1[] = {
    /* TODO: Uncomment once TF-RMM fix the issue */
//    {.msg = "vdev_id",
//    .abi = RSI_VDEV_GET_INFO,
//    .label = VDEV_IDS_INVALID_VDEV_ID,
//    .status = RSI_ERROR_INPUT,
//    .index = 0},
    {.msg = "addr_align",
    .abi = RSI_VDEV_GET_INFO,
    .label = ADDR_UNALIGNED,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "addr_bound",
    .abi = RSI_VDEV_GET_INFO,
    .label = ADDR_UNPROTECTED,
    .status = RSI_ERROR_INPUT,
    .index = 0},
//    {.msg = "addr_empty",
//    .abi = RSI_VDEV_GET_INFO,
//    .label = ADDR_RIPAS_EMPTY,
//    .status = RSI_ERROR_INPUT,
//    .index = 0}
};
