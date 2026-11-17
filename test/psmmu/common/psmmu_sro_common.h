/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef PSMMU_SRO_COMMON_H
#define PSMMU_SRO_COMMON_H

#include "val.h"
#include "val_host_realm.h"
#include "val_host_da.h"
#include "val_host_rhi.h"
#include "val_host_rmi.h"

#define PSMMU_SRO_L2_SID            (0UL)
#define PSMMU_SRO_DPT_L1_ALIGN      (1UL << 30)
#define PSMMU_SRO_DPT_L1_ADDR       (0UL)

typedef struct {
    uint32_t saw_incomplete;
    uint32_t saw_donate;
    uint32_t saw_reclaim;
    uint32_t saw_alignment_failure;
} psmmu_sro_result_ts;

typedef struct {
    uint64_t psmmu;
    val_host_realm_ts realm;
    val_host_pdev_ts pdev;
    val_host_vdev_ts vdev;
    val_host_psmmu_info_ts *info;
    val_host_psmmu_params_ts *params;
    uint32_t activated;
    uint32_t l0_created;
    uint32_t l1_created;
    uint32_t l2_created;
    uint32_t da_ready;
    uint64_t l1_addr;
    uint64_t sid;
} psmmu_sro_context_ts;

uint32_t psmmu_sro_da_feature_supported(void);
uint64_t psmmu_sro_make_flags(uint64_t irq_cfg, uint64_t cmdq_sync_irq_wired,
                              uint64_t ats, uint64_t pri);
uint32_t psmmu_sro_context_init_psmmu_only(psmmu_sro_context_ts *ctx);
uint32_t psmmu_sro_context_init(psmmu_sro_context_ts *ctx);
void psmmu_sro_context_cleanup(psmmu_sro_context_ts *ctx);
uint64_t psmmu_sro_complete(val_smc_param_ts args, uint32_t try_align_negative,
                            psmmu_sro_result_ts *sro);
uint64_t psmmu_sro_activate(psmmu_sro_context_ts *ctx, uint32_t ats,
                            psmmu_sro_result_ts *sro);
uint64_t psmmu_sro_deactivate(psmmu_sro_context_ts *ctx,
                              psmmu_sro_result_ts *sro);
uint64_t psmmu_sro_dpt_l0_create(psmmu_sro_context_ts *ctx,
                                 uint32_t try_align_negative,
                                 psmmu_sro_result_ts *sro);
uint64_t psmmu_sro_dpt_l0_destroy(psmmu_sro_context_ts *ctx,
                                  psmmu_sro_result_ts *sro);
uint64_t psmmu_sro_dpt_l1_create(psmmu_sro_context_ts *ctx, uint64_t addr,
                                 psmmu_sro_result_ts *sro);
uint64_t psmmu_sro_dpt_l1_destroy(psmmu_sro_context_ts *ctx, uint64_t addr,
                                  psmmu_sro_result_ts *sro);
uint64_t psmmu_sro_st_l2_create(psmmu_sro_context_ts *ctx, uint64_t sid,
                                psmmu_sro_result_ts *sro);
uint64_t psmmu_sro_st_l2_destroy(psmmu_sro_context_ts *ctx, uint64_t sid,
                                 psmmu_sro_result_ts *sro);
uint64_t psmmu_sro_activate_prepare(psmmu_sro_context_ts *ctx,
                                    psmmu_sro_result_ts *sro);

#endif /* PSMMU_SRO_COMMON_H */
