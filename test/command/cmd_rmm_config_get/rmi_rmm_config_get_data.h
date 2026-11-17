/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    CFG_UNALIGNED = 0X0,
    CFG_PAS_REALM = 0X1,
    CFG_PAS_SECURE = 0X2,
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "cfg_align",
    .abi = RMI_RMM_CONFIG_GET,
    .label = CFG_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "cfg_pas",
    .abi = RMI_RMM_CONFIG_GET,
    .label = CFG_PAS_REALM,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "cfg_pas",
    .abi = RMI_RMM_CONFIG_GET,
    .label = CFG_PAS_SECURE,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
