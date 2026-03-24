/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    PDEV_UNALIGNED = 0X0,
    PDEV_OUTSIDE_OF_PERMITTED_PA = 0X1,
    PDEV_DEV_MEM_MMIO = 0X2,
    PDEV_GRAN_STATE_UNDELEGATED = 0X3,
    PARAMS_UNALIGNED = 0X4,
    PARAMS_PAS_REALM = 0X5,
    PARAMS_PAS_SECURE = 0X6,
    INVALID_KEY_LEN = 0X7,
    INVALID_METADATA_LEN = 0X8,
    INVALID_KEY = 0X9,
    INVALID_METADATA = 0XA,
    PDEV_STATE_PDEV_NEW = 0XB
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "pdev_align",
    .abi = RMI_PDEV_SET_PUBKEY,
    .label = PDEV_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pdev_bound",
    .abi = RMI_PDEV_SET_PUBKEY,
    .label = PDEV_OUTSIDE_OF_PERMITTED_PA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pdev_bound",
    .abi = RMI_PDEV_SET_PUBKEY,
    .label = PDEV_DEV_MEM_MMIO,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pdev_gran_state",
    .abi = RMI_PDEV_SET_PUBKEY,
    .label = PDEV_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "params_align",
    .abi = RMI_PDEV_SET_PUBKEY,
    .label = PARAMS_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "params_pas",
    .abi = RMI_PDEV_SET_PUBKEY,
    .label = PARAMS_PAS_REALM,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "params_pas",
    .abi = RMI_PDEV_SET_PUBKEY,
    .label = PARAMS_PAS_SECURE,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "key_len_oflow",
    .abi = RMI_PDEV_SET_PUBKEY,
    .label = INVALID_KEY_LEN,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "metadata_len_oflow",
    .abi = RMI_PDEV_SET_PUBKEY,
    .label = INVALID_METADATA_LEN,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "key_invalid",
    .abi = RMI_PDEV_SET_PUBKEY,
    .label = INVALID_KEY,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    /* TODO: Uncomment once TF-RMM fix the issue */
    // {.msg = "metadata_invalid",
    // .abi = RMI_PDEV_SET_PUBKEY,
    // .label = INVALID_METADATA,
    // .status = RMI_ERROR_INPUT,
    // .index = 0},
    {.msg = "pdev_state",
    .abi = RMI_PDEV_SET_PUBKEY,
    .label = PDEV_STATE_PDEV_NEW,
    .status = RMI_ERROR_DEVICE,
    .index = 0}
};
