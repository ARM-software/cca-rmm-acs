/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    psmmu_valid = 0x0,
    psmmu_state,
    params_align,
    params_pas,
    irq_cfg,
    cmdq_sync_irq_wired,
    gerror_valid,
    eventq_valid,
    priq_valid,
    ats_supp,
    pri_supp,
    dpt
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "psmmu_valid",
    .abi = RMI_PSMMU_ACTIVATE,
    .label = psmmu_valid,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "params_align",
    .abi = RMI_PSMMU_ACTIVATE,
    .label = params_align,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "params_pas",
    .abi = RMI_PSMMU_ACTIVATE,
    .label = params_pas,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "irq_cfg",
    .abi = RMI_PSMMU_ACTIVATE,
    .label = irq_cfg,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "cmdq_sync_irq_wired",
    .abi = RMI_PSMMU_ACTIVATE,
    .label = cmdq_sync_irq_wired,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "gerror_valid",
    .abi = RMI_PSMMU_ACTIVATE,
    .label = gerror_valid,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "eventq_valid",
    .abi = RMI_PSMMU_ACTIVATE,
    .label = eventq_valid,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "priq_valid",
    .abi = RMI_PSMMU_ACTIVATE,
    .label = priq_valid,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "ats_supp",
    .abi = RMI_PSMMU_ACTIVATE,
    .label = ats_supp,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "pri_supp",
    .abi = RMI_PSMMU_ACTIVATE,
    .label = pri_supp,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "dpt",
    .abi = RMI_PSMMU_ACTIVATE,
    .label = dpt,
    .status = RMI_ERROR_DPT,
    .index = 0},
    {.msg = "psmmu_state",
    .abi = RMI_PSMMU_ACTIVATE,
    .label = psmmu_state,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
