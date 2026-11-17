/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "psmmu_command_common_host.h"
#include "test_database.h"
#include "val_host_sro.h"

uint64_t psmmu_cmd_make_flags(uint64_t irq_cfg, uint64_t cmdq_sync_irq_wired,
                              uint64_t ats, uint64_t pri)
{
    return (irq_cfg & 0x3U) |
           ((cmdq_sync_irq_wired & 0x1U) << 2U) |
           ((ats & 0x1U) << 3U) |
           ((pri & 0x1U) << 4U);
}

uint32_t psmmu_cmd_context_init(psmmu_cmd_context_ts *ctx)
{
    val_smc_param_ts args;

    val_memset(ctx, 0, sizeof(*ctx));
    ctx->psmmu = PSMMU_FVP_BASE_ADDR;

    if (!val_host_rmm_supports_da()) {
        LOG(ALWAYS, "DA feature not supported\n");
        return VAL_SKIP_CHECK;
    }

    ctx->info = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    ctx->params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((ctx->info == NULL) || (ctx->params == NULL)) {
        LOG(ERROR, "Failed to allocate PSMMU info/params.\n");
        psmmu_cmd_context_cleanup(ctx);
        return VAL_ERROR;
    }

    val_memset(ctx->info, 0, PAGE_SIZE_4K);
    val_memset(ctx->params, 0, PAGE_SIZE_4K);

    args = val_host_rmi_psmmu_info(ctx->psmmu, (uint64_t)ctx->info);
    if (args.x0 == RMI_ERROR_NOT_SUPPORTED) {
        psmmu_cmd_context_cleanup(ctx);
        return VAL_SKIP_CHECK;
    }
    if (args.x0 != RMI_SUCCESS) {
        LOG(ERROR, "RMI_PSMMU_INFO failed, ret=0x%lx.\n", args.x0);
        psmmu_cmd_context_cleanup(ctx);
        return VAL_ERROR;
    }

    if ((ctx->info->flags & 0x3U) == RMI_IRQ_MSI) {
        LOG(ERROR, "MSI IRQ config requires platform MSI addresses, skipping.\n");
        psmmu_cmd_context_cleanup(ctx);
        return VAL_SKIP_CHECK;
    }

    ctx->sid = 0UL;
    ctx->l1_addr = PSMMU_CMD_DPT_L0_ALIGN;

    return VAL_SUCCESS;
}

void psmmu_cmd_da_teardown(psmmu_cmd_context_ts *ctx)
{
    if (ctx->da_ready == 0U) {
        return;
    }

    if (val_host_vdev_teardown(&ctx->realm, &ctx->pdev, &ctx->vdev) !=
        VAL_SUCCESS) {
        LOG(ERROR, "VDEV teardown failed.\n");
    }
    if (val_host_pdev_teardown(&ctx->pdev, ctx->pdev.pdev) != VAL_SUCCESS) {
        LOG(ERROR, "PDEV teardown failed.\n");
    }
    if (val_host_realm_destroy(ctx->realm.rd) != VAL_SUCCESS) {
        LOG(ERROR, "Realm destroy failed.\n");
    }
    ctx->da_ready = 0;
}

void psmmu_cmd_context_cleanup(psmmu_cmd_context_ts *ctx)
{
    val_smc_param_ts args;
    uint32_t i;

    if (ctx->l2_created != 0U) {
        (void)val_host_rmi_psmmu_st_l2_destroy(ctx->psmmu, ctx->sid);
        ctx->l2_created = 0U;
    }
    if (ctx->activated != 0U) {
        (void)val_host_rmi_psmmu_deactivate(ctx->psmmu);
        ctx->activated = 0U;
    }
    if (ctx->l1_created != 0U) {
        args = val_host_rmi_dpt_l1_destroy(ctx->l1_addr);
        (void)val_host_sro_complete(&args);
        ctx->l1_created = 0U;
    }
    if (ctx->l0_created != 0U) {
        args = val_host_rmi_dpt_l0_destroy();
        (void)val_host_sro_complete(&args);
        ctx->l0_created = 0U;
    }

    psmmu_cmd_da_teardown(ctx);

    for (i = 0U; i < ctx->params_alloc_count; i++) {
        if (ctx->params_allocs[i] != 0U) {
            val_host_mem_free((void *)ctx->params_allocs[i]);
            ctx->params_allocs[i] = 0U;
        }
    }
    ctx->params_alloc_count = 0U;

    if (ctx->params != NULL) {
        val_host_mem_free(ctx->params);
        ctx->params = NULL;
    }
    if (ctx->info != NULL) {
        val_host_mem_free(ctx->info);
        ctx->info = NULL;
    }
}

uint32_t psmmu_cmd_dpt_not_supported(uint64_t ret)
{
    return ((ret == RMI_ERROR_NOT_SUPPORTED) || (ret == VAL_SMC_NOT_SUPPORTED));
}

uint32_t psmmu_cmd_ats_supported(void)
{
    uint64_t featreg2;
    uint64_t ats;

    if (val_host_rmi_features(RMI_FEATURE_REGISTER_2_INDEX, &featreg2) !=
        RMI_SUCCESS) {
        return 0U;
    }

    ats = VAL_EXTRACT_BITS(featreg2, RMI_FEATURE_REGISTER_2_ATS,
                           RMI_FEATURE_REGISTER_2_ATS);
    return (ats == RMI_FEATURE_TRUE) ? 1U : 0U;
}

uint64_t psmmu_cmd_params_prep_sequence(psmmu_cmd_context_ts *ctx,
                                        psmmu_cmd_params_type_te type)
{
    val_host_psmmu_params_ts *params;
    uint64_t irq_cfg = ctx->info->flags & 0x3U;
    uint64_t cmdq_sync = (ctx->info->flags >> 2U) & 0x1U;

    if (ctx->params_alloc_count >= PSMMU_CMD_PARAMS_TRACK_MAX) {
        LOG(ERROR, "Too many PSMMU params allocations.\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (params == NULL) {
        LOG(ERROR, "Failed to allocate PSMMU params.\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    val_memset(params, 0, PAGE_SIZE_4K);
    params->flags = psmmu_cmd_make_flags(irq_cfg, cmdq_sync,
                                         RMI_FEATURE_FALSE,
                                         RMI_FEATURE_FALSE);

    switch (type) {
    case PSMMU_CMD_PARAMS_VALID:
        break;
    case PSMMU_CMD_PARAMS_IRQ_INVALID:
        params->flags = psmmu_cmd_make_flags(0x3U, cmdq_sync,
                                             RMI_FEATURE_FALSE,
                                             RMI_FEATURE_FALSE);
        break;
    case PSMMU_CMD_PARAMS_CMDQ_SYNC_INVALID:
        params->flags = psmmu_cmd_make_flags(irq_cfg, !cmdq_sync,
                                             RMI_FEATURE_FALSE,
                                             RMI_FEATURE_FALSE);
        break;
    case PSMMU_CMD_PARAMS_MSI_GERROR_INVALID:
        params->flags = psmmu_cmd_make_flags(RMI_IRQ_MSI, cmdq_sync,
                                             RMI_FEATURE_FALSE,
                                             RMI_FEATURE_FALSE);
        params->gerr_addr = PSMMU_CMD_INVALID_BOUND;
        break;
    case PSMMU_CMD_PARAMS_MSI_EVENTQ_INVALID:
        params->flags = psmmu_cmd_make_flags(RMI_IRQ_MSI, cmdq_sync,
                                             RMI_FEATURE_FALSE,
                                             RMI_FEATURE_FALSE);
        params->eventq_addr = PSMMU_CMD_INVALID_BOUND;
        break;
    case PSMMU_CMD_PARAMS_MSI_PRIQ_INVALID:
        params->flags = psmmu_cmd_make_flags(RMI_IRQ_MSI, cmdq_sync,
                                             RMI_FEATURE_FALSE,
                                             RMI_FEATURE_FALSE);
        params->priq_addr = PSMMU_CMD_INVALID_BOUND;
        break;
    case PSMMU_CMD_PARAMS_ATS_TRUE:
        params->flags = psmmu_cmd_make_flags(irq_cfg, cmdq_sync,
                                             RMI_FEATURE_TRUE,
                                             RMI_FEATURE_FALSE);
        break;
    case PSMMU_CMD_PARAMS_PRI_TRUE:
        params->flags = psmmu_cmd_make_flags(irq_cfg, cmdq_sync,
                                             RMI_FEATURE_FALSE,
                                             RMI_FEATURE_TRUE);
        break;
    default:
        val_host_mem_free(params);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    ctx->params_allocs[ctx->params_alloc_count] = (uint64_t)params;
    ctx->params_alloc_count++;

    return (uint64_t)params;
}

uint32_t psmmu_cmd_expect_status(uint64_t got, uint64_t expected,
                                 uint32_t error_point)
{
    if (got != expected) {
        LOG(ERROR, "Unexpected command result 0x%lx, expected 0x%lx.\n",
            got, expected);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(error_point)));
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

uint64_t psmmu_cmd_sid_unaligned(psmmu_cmd_context_ts *ctx)
{
    return ctx->sid + 1U;
}

uint64_t psmmu_cmd_sid_out_of_range(void)
{
    return PSMMU_CMD_INVALID_BOUND;
}

uint64_t psmmu_cmd_dpt_addr_unaligned(psmmu_cmd_context_ts *ctx)
{
    return ctx->l1_addr + PAGE_SIZE;
}

uint64_t psmmu_cmd_dpt_addr_out_of_range(void)
{
    return PSMMU_CMD_DPT_BOUND;
}
