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
    DATA_UNALIGNED = 0X4,
    DATA_PAS_REALM = 0X5,
    DATA_PAS_SECURE = 0X6,
    REQ_UNALIGNED = 0X7,
    REQ_PAS_REALM = 0X8,
    REQ_PAS_SECURE = 0X9,
    RESP_UNALIGNED = 0XA,
    RESP_PAS_REALM = 0XB,
    RESP_PAS_SECURE = 0XC,
    INVALID_RESP_LEN = 0XD
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
    .abi = RMI_PDEV_COMMUNICATE,
    .label = PDEV_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pdev_bound",
    .abi = RMI_PDEV_COMMUNICATE,
    .label = PDEV_OUTSIDE_OF_PERMITTED_PA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pdev_bound",
    .abi = RMI_PDEV_COMMUNICATE,
    .label = PDEV_DEV_MEM_MMIO,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pdev_gran_state",
    .abi = RMI_PDEV_COMMUNICATE,
    .label = PDEV_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "data_align",
    .abi = RMI_PDEV_COMMUNICATE,
    .label = DATA_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "data_pas",
    .abi = RMI_PDEV_COMMUNICATE,
    .label = DATA_PAS_REALM,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "data_pas",
    .abi = RMI_PDEV_COMMUNICATE,
    .label = DATA_PAS_SECURE,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "req_align",
    .abi = RMI_PDEV_COMMUNICATE,
    .label = REQ_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "req_pas",
    .abi = RMI_PDEV_COMMUNICATE,
    .label = REQ_PAS_REALM,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "req_pas",
    .abi = RMI_PDEV_COMMUNICATE,
    .label = REQ_PAS_SECURE,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "resp_align",
    .abi = RMI_PDEV_COMMUNICATE,
    .label = RESP_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "resp_pas",
    .abi = RMI_PDEV_COMMUNICATE,
    .label = RESP_PAS_REALM,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "resp_pas",
    .abi = RMI_PDEV_COMMUNICATE,
    .label = RESP_PAS_SECURE,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "resp_len",
    .abi = RMI_PDEV_COMMUNICATE,
    .label = INVALID_RESP_LEN,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
