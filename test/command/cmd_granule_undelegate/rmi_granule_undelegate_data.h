/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    ADDR_UNALIGNED = 0X0,
    ADDR_DEV_MEM_MMIO = 0X1,
    ADDR_OUTSIDE_OF_PERMITTED_PA = 0X2,
    ADDR_UNDELEGATED = 0X3,
    ADDR_REC = 0X4,
    ADDR_DATA = 0X5,
    ADDR_RTT = 0X6,
    ADDR_RD = 0X7
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "gran_align",
    .abi = RMI_GRANULE_UNDELEGATE,
    .label = ADDR_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "gran_bound",
    .abi = RMI_GRANULE_UNDELEGATE,
    .label = ADDR_DEV_MEM_MMIO,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "gran_bound",
    .abi = RMI_GRANULE_UNDELEGATE,
    .label = ADDR_OUTSIDE_OF_PERMITTED_PA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "gran_state",
    .abi = RMI_GRANULE_UNDELEGATE,
    .label = ADDR_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "gran_state",
    .abi = RMI_GRANULE_UNDELEGATE,
    .label = ADDR_REC,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "gran_state",
    .abi = RMI_GRANULE_UNDELEGATE,
    .label = ADDR_DATA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "gran_state",
    .abi = RMI_GRANULE_UNDELEGATE,
    .label = ADDR_RTT,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "gran_state",
    .abi = RMI_GRANULE_UNDELEGATE,
    .label = ADDR_RD,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
