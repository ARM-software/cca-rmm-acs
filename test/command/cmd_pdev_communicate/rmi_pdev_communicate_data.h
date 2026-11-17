/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    PDEV_UNALIGNED = 0X0,
    PDEV_GRAN_STATE_UNDELEGATED = 0X1,
    DATA_UNALIGNED = 0X2,
    DATA_PAS_REALM = 0X3,
    DATA_PAS_SECURE = 0X4,
    REQ_UNALIGNED = 0X5,
    REQ_PAS_REALM = 0X6,
    REQ_PAS_SECURE = 0X7,
    RSP_UNALIGNED = 0X8,
    RSP_PAS_REALM = 0X9,
    RSP_PAS_SECURE = 0XA,
    INVALID_RSP_LEN = 0XB,
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
    {.msg = "rsp_align",
    .abi = RMI_PDEV_COMMUNICATE,
    .label = RSP_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rsp_pas",
    .abi = RMI_PDEV_COMMUNICATE,
    .label = RSP_PAS_REALM,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rsp_pas",
    .abi = RMI_PDEV_COMMUNICATE,
    .label = RSP_PAS_SECURE,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rsp_len",
    .abi = RMI_PDEV_COMMUNICATE,
    .label = INVALID_RSP_LEN,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
