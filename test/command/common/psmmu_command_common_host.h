/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef PSMMU_COMMAND_COMMON_HOST_H
#define PSMMU_COMMAND_COMMON_HOST_H

#include "val.h"
#include "val_host_da.h"
#include "val_host_realm.h"
#include "val_host_rhi.h"
#include "val_host_rmi.h"
#include "val_host_sro.h"
#include "val_smc.h"

#define PSMMU_CMD_L2_STE_COUNT      (64UL)
#define PSMMU_CMD_INVALID_BOUND     (1UL << 63)
#define PSMMU_CMD_DPT_L0_ALIGN      (1UL << 30)
#define PSMMU_CMD_DPT_BOUND         (1UL << 48)
#define PSMMU_CMD_PARAMS_TRACK_MAX  (32U)


typedef struct {
    uint64_t psmmu;
    val_host_realm_ts realm;
    val_host_pdev_ts pdev;
    val_host_vdev_ts vdev;
    val_host_psmmu_info_ts *info;
    val_host_psmmu_params_ts *params;
    uint64_t params_allocs[PSMMU_CMD_PARAMS_TRACK_MAX];
    uint32_t params_alloc_count;
    uint32_t da_ready;
    uint32_t activated;
    uint32_t l0_created;
    uint32_t l1_created;
    uint32_t l2_created;
    uint64_t l1_addr;
    uint64_t sid;
} psmmu_cmd_context_ts;

typedef enum {
    PSMMU_CMD_PARAMS_VALID = 0x0,
    PSMMU_CMD_PARAMS_IRQ_INVALID,
    PSMMU_CMD_PARAMS_CMDQ_SYNC_INVALID,
    PSMMU_CMD_PARAMS_MSI_GERROR_INVALID,
    PSMMU_CMD_PARAMS_MSI_EVENTQ_INVALID,
    PSMMU_CMD_PARAMS_MSI_PRIQ_INVALID,
    PSMMU_CMD_PARAMS_ATS_TRUE,
    PSMMU_CMD_PARAMS_PRI_TRUE
} psmmu_cmd_params_type_te;

uint32_t psmmu_cmd_context_init(psmmu_cmd_context_ts *ctx);
void psmmu_cmd_context_cleanup(psmmu_cmd_context_ts *ctx);
void psmmu_cmd_da_teardown(psmmu_cmd_context_ts *ctx);
uint64_t psmmu_cmd_make_flags(uint64_t irq_cfg, uint64_t cmdq_sync_irq_wired,
                              uint64_t ats, uint64_t pri);
uint32_t psmmu_cmd_dpt_not_supported(uint64_t ret);
uint32_t psmmu_cmd_ats_supported(void);
uint32_t psmmu_cmd_expect_status(uint64_t got, uint64_t expected,
                                 uint32_t error_point);
uint64_t psmmu_cmd_params_prep_sequence(psmmu_cmd_context_ts *ctx,
                                        psmmu_cmd_params_type_te type);
uint64_t psmmu_cmd_sid_unaligned(psmmu_cmd_context_ts *ctx);
uint64_t psmmu_cmd_sid_out_of_range(void);
uint64_t psmmu_cmd_dpt_addr_unaligned(psmmu_cmd_context_ts *ctx);
uint64_t psmmu_cmd_dpt_addr_out_of_range(void);

#endif /* PSMMU_COMMAND_COMMON_HOST_H */
