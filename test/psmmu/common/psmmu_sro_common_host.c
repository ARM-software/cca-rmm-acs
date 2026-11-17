/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "psmmu_sro_common.h"
#include "val_host_sro.h"
#include "val_host_rmi_sro.h"

#define PSMMU_SRO_DONATE_REQ_CONTIG(req) VAL_EXTRACT_BITS((req), 16, 16)

static uint32_t is_power_of_two_u64(uint64_t value)
{
    return (value != 0U) && ((value & (value - 1U)) == 0U);
}

static uint64_t psmmu_sro_addr_range_desc(uint64_t base, uint64_t count,
                                          uint64_t state)
{
    return RMI_PAGE_L3 |
           (count << 2) |
           ADDR_ALIGN_DOWN(base, PAGE_SIZE) |
           (state << 62);
}

static uint64_t psmmu_sro_try_misaligned_donation(uint64_t handle,
                                                  uint64_t donate_req)
{
    uint64_t size = VAL_HOST_SRO_DONATE_REQ_SIZE(donate_req);
    uint64_t count = VAL_HOST_SRO_DONATE_REQ_COUNT(donate_req);
    uint64_t contig = PSMMU_SRO_DONATE_REQ_CONTIG(donate_req);
    uint64_t state = VAL_HOST_SRO_DONATE_REQ_STATE(donate_req);
    uint64_t donated_size;
    uint64_t list_addr = 0U;
    uint64_t mem_alloc_base = 0U;
    uint64_t mem_base = 0U;
    uint64_t ret;
    val_smc_param_ts args;

    if ((size != RMI_PAGE_L3) || (count == 0U) ||
        (contig != RMI_OP_MEM_CONTIG) ||
        (count > VAL_HOST_SRO_DESC_MAX_COUNT)) {
        return RMI_INCOMPLETE;
    }

    donated_size = count * PAGE_SIZE;
    if (((donated_size / PAGE_SIZE) != count) ||
        (donated_size <= PAGE_SIZE) ||
        !is_power_of_two_u64(donated_size)) {
        return RMI_INCOMPLETE;
    }

    list_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    mem_alloc_base = (uint64_t)val_host_mem_alloc(donated_size,
                                                  donated_size + PAGE_SIZE);
    if ((list_addr == 0U) || (mem_alloc_base == 0U)) {
        LOG(ERROR, "Failed to allocate misaligned SRO donation memory.\n");
        ret = RMI_INCOMPLETE;
        goto cleanup;
    }

    mem_base = mem_alloc_base + PAGE_SIZE;
    if ((state == RMI_OP_MEM_DELEGATED) &&
        (val_host_rmi_granule_range_delegate(mem_base,
                                             mem_base + donated_size) != RMI_SUCCESS)) {
        LOG(ERROR, "Failed to delegate misaligned SRO donation granules.\n");
        ret = RMI_ERROR_INPUT;
        goto cleanup;
    }

    val_memset((void *)list_addr, 0, PAGE_SIZE);
    ((uint64_t *)list_addr)[0] = psmmu_sro_addr_range_desc(mem_base, count,
                                                           state);

    args = val_host_rmi_op_mem_donate_smc(handle, list_addr, 1);
    if (RMI_STATUS(args.x0) != RMI_ERROR_INPUT) {
        LOG(ERROR, "Misaligned SRO donation did not fail.\n");
        (void)val_host_rmi_op_continue_smc(handle, RMI_CONTINUE_KEEP_GOING);
        ret = RMI_SUCCESS;
        goto cleanup;
    }

    (void)val_host_rmi_op_continue_smc(handle, RMI_CONTINUE_KEEP_GOING);
    ret = RMI_ERROR_INPUT;

cleanup:
    if ((state == RMI_OP_MEM_DELEGATED) && (mem_base != 0U)) {
        (void)val_host_rmi_granule_range_undelegate(mem_base,
                                                    mem_base + donated_size);
    }
    if (mem_alloc_base != 0U) {
        val_host_mem_free((void *)mem_alloc_base);
    }
    if (list_addr != 0U) {
        val_host_mem_free((void *)list_addr);
    }

    return ret;
}

uint32_t psmmu_sro_da_feature_supported(void)
{
    if (!val_host_rmm_supports_da()) {
        LOG(ERROR, "DA feature not supported.\n");
        return 0;
    }

    return 1;
}

uint64_t psmmu_sro_make_flags(uint64_t irq_cfg, uint64_t cmdq_sync_irq_wired,
                              uint64_t ats, uint64_t pri)
{
    return (irq_cfg & 0x3U) |
           ((cmdq_sync_irq_wired & 0x1U) << 2U) |
           ((ats & 0x1U) << 3U) |
           ((pri & 0x1U) << 4U);
}

static uint32_t psmmu_sro_context_init_common(psmmu_sro_context_ts *ctx,
                                                uint32_t setup_da)
{
    val_smc_param_ts args;

    val_memset(ctx, 0, sizeof(*ctx));
    ctx->psmmu = PSMMU_FVP_BASE_ADDR;
    ctx->l1_addr = PSMMU_SRO_DPT_L1_ADDR;
    ctx->sid = PSMMU_SRO_L2_SID;

    if (!psmmu_sro_da_feature_supported()) {
        return VAL_SKIP_CHECK;
    }

    ctx->info = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    ctx->params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((ctx->info == NULL) || (ctx->params == NULL)) {
        LOG(ERROR, "Failed to allocate PSMMU info/params.\n");
        psmmu_sro_context_cleanup(ctx);
        return VAL_ERROR;
    }

    val_memset(ctx->info, 0, PAGE_SIZE_4K);
    val_memset(ctx->params, 0, PAGE_SIZE_4K);

    args = val_host_rmi_psmmu_info(ctx->psmmu, (uint64_t)ctx->info);
    if (args.x0 == RMI_ERROR_NOT_SUPPORTED) {
        LOG(ERROR, "PSMMU info not supported.\n");
        psmmu_sro_context_cleanup(ctx);
        return VAL_SKIP_CHECK;
    }
    if (args.x0 != RMI_SUCCESS) {
        LOG(ERROR, "PSMMU info failed, ret=0x%lx.\n", args.x0);
        psmmu_sro_context_cleanup(ctx);
        return VAL_ERROR;
    }

    if ((ctx->info->flags & 0x3U) == RMI_IRQ_MSI) {
        LOG(ERROR, "MSI IRQ config requires platform MSI addresses, skipping.\n");
        psmmu_sro_context_cleanup(ctx);
        return VAL_SKIP_CHECK;
    }

    if (setup_da == 0U) {
        return VAL_SUCCESS;
    }

    if (da_init(&ctx->realm, &ctx->pdev, &ctx->vdev, RMI_VDEV_LOCKED) !=
        VAL_SUCCESS) {
        LOG(ERROR, "DA realm/PDEV/VDEV setup failed.\n");
        return VAL_SKIP_CHECK;
    }
    ctx->da_ready = 1;
    ctx->sid = ((uint64_t)ctx->pdev.bdf) & ~0x3FULL;
    if (ctx->pdev.ncoh_num_addr_range == 0U) {
        LOG(ERROR, "No assigned-device memory ranges reported.\n");
        psmmu_sro_context_cleanup(ctx);
        return VAL_SKIP_CHECK;
    }
    ctx->l1_addr = ADDR_ALIGN_DOWN(ctx->pdev.ncoh_addr_range[0].base,
                                   PSMMU_SRO_DPT_L1_ALIGN);

    return VAL_SUCCESS;
}

uint32_t psmmu_sro_context_init_psmmu_only(psmmu_sro_context_ts *ctx)
{
    return psmmu_sro_context_init_common(ctx, 0U);
}

uint32_t psmmu_sro_context_init(psmmu_sro_context_ts *ctx)
{
    return psmmu_sro_context_init_common(ctx, 1U);
}

void psmmu_sro_context_cleanup(psmmu_sro_context_ts *ctx)
{
    psmmu_sro_result_ts sro;

    if (ctx->l2_created != 0U) {
        (void)psmmu_sro_st_l2_destroy(ctx, ctx->sid, &sro);
    }
    if (ctx->activated != 0U) {
        (void)psmmu_sro_deactivate(ctx, &sro);
    }
    if (ctx->l1_created != 0U) {
        (void)psmmu_sro_dpt_l1_destroy(ctx, ctx->l1_addr, &sro);
    }
    if (ctx->l0_created != 0U) {
        (void)psmmu_sro_dpt_l0_destroy(ctx, &sro);
    }
    if (ctx->da_ready != 0U) {
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
    if (ctx->params != NULL) {
        val_host_mem_free(ctx->params);
        ctx->params = NULL;
    }
    if (ctx->info != NULL) {
        val_host_mem_free(ctx->info);
        ctx->info = NULL;
    }
}

uint64_t psmmu_sro_complete(val_smc_param_ts args, uint32_t try_align_negative,
                            psmmu_sro_result_ts *sro)
{
    uint64_t mem_req;
    uint64_t ret;

    if (sro != NULL) {
        val_memset(sro, 0, sizeof(*sro));
    }

    if (RMI_STATUS(args.x0) != RMI_INCOMPLETE) {
        return args.x0;
    }

    mem_req = VAL_HOST_SRO_MEM_REQ(args.x0);
    if (sro != NULL) {
        sro->saw_incomplete = 1;
        sro->saw_donate = (mem_req == RMI_OP_MEM_REQ_DONATE) ? 1U : 0U;
        sro->saw_reclaim = (mem_req == RMI_OP_MEM_REQ_RECLAIM) ? 1U : 0U;
    }

    if ((try_align_negative != 0U) && (mem_req == RMI_OP_MEM_REQ_DONATE)) {
        ret = psmmu_sro_try_misaligned_donation(args.x1, args.x2);
        if ((ret == RMI_ERROR_INPUT) && (sro != NULL)) {
            sro->saw_alignment_failure = 1;
        }
        return ret;
    }

    return val_host_sro_complete(&args);
}

uint64_t psmmu_sro_activate(psmmu_sro_context_ts *ctx, uint32_t ats,
                            psmmu_sro_result_ts *sro)
{
    uint64_t irq_cfg = ctx->info->flags & 0x3U;
    uint64_t cmdq_sync = (ctx->info->flags >> 2U) & 0x1U;
    uint64_t ret;

    val_memset(ctx->params, 0, PAGE_SIZE_4K);
    ctx->params->flags = psmmu_sro_make_flags(irq_cfg, cmdq_sync, ats,
                                              RMI_FEATURE_FALSE);

    ret = psmmu_sro_complete(
        val_host_rmi_psmmu_activate_smc(ctx->psmmu, (uint64_t)ctx->params),
        0, sro);
    if (ret == RMI_SUCCESS) {
        ctx->activated = 1;
    }

    return ret;
}

uint64_t psmmu_sro_deactivate(psmmu_sro_context_ts *ctx,
                              psmmu_sro_result_ts *sro)
{
    uint64_t ret;

    ret = psmmu_sro_complete(val_host_rmi_psmmu_deactivate_smc(ctx->psmmu),
                             0, sro);
    if (ret == RMI_SUCCESS) {
        ctx->activated = 0;
    }

    return ret;
}

uint64_t psmmu_sro_dpt_l0_create(psmmu_sro_context_ts *ctx,
                                 uint32_t try_align_negative,
                                 psmmu_sro_result_ts *sro)
{
    uint64_t ret;

    ret = psmmu_sro_complete(val_host_rmi_dpt_l0_create(),
                             try_align_negative, sro);
    if (ret == RMI_SUCCESS) {
        ctx->l0_created = 1;
    }

    return ret;
}

uint64_t psmmu_sro_dpt_l0_destroy(psmmu_sro_context_ts *ctx,
                                  psmmu_sro_result_ts *sro)
{
    uint64_t ret;

    ret = psmmu_sro_complete(val_host_rmi_dpt_l0_destroy(), 0, sro);
    if (ret == RMI_SUCCESS) {
        ctx->l0_created = 0;
    }

    return ret;
}

uint64_t psmmu_sro_dpt_l1_create(psmmu_sro_context_ts *ctx, uint64_t addr,
                                 psmmu_sro_result_ts *sro)
{
    uint64_t ret;

    ret = psmmu_sro_complete(val_host_rmi_dpt_l1_create(addr), 0, sro);
    if (ret == RMI_SUCCESS) {
        ctx->l1_created = 1;
        ctx->l1_addr = addr;
    }

    return ret;
}

uint64_t psmmu_sro_dpt_l1_destroy(psmmu_sro_context_ts *ctx, uint64_t addr,
                                  psmmu_sro_result_ts *sro)
{
    uint64_t ret;

    ret = psmmu_sro_complete(val_host_rmi_dpt_l1_destroy(addr), 0, sro);
    if (ret == RMI_SUCCESS) {
        ctx->l1_created = 0;
    }

    return ret;
}

uint64_t psmmu_sro_st_l2_create(psmmu_sro_context_ts *ctx, uint64_t sid,
                                psmmu_sro_result_ts *sro)
{
    uint64_t ret;

    ret = psmmu_sro_complete(
        val_host_rmi_psmmu_st_l2_create_smc(ctx->psmmu, sid), 0, sro);
    if (ret == RMI_SUCCESS) {
        ctx->l2_created = 1;
        ctx->sid = sid;
    }

    return ret;
}

uint64_t psmmu_sro_st_l2_destroy(psmmu_sro_context_ts *ctx, uint64_t sid,
                                 psmmu_sro_result_ts *sro)
{
    uint64_t ret;

    ret = psmmu_sro_complete(
        val_host_rmi_psmmu_st_l2_destroy_smc(ctx->psmmu, sid), 0, sro);
    if (ret == RMI_SUCCESS) {
        ctx->l2_created = 0;
    }

    return ret;
}

uint64_t psmmu_sro_activate_prepare(psmmu_sro_context_ts *ctx,
                                    psmmu_sro_result_ts *sro)
{
    uint64_t ret;
    psmmu_sro_result_ts tmp;

    ret = psmmu_sro_activate(ctx, RMI_FEATURE_TRUE, sro);
    if (ret == RMI_ERROR_DPT) {
        ret = psmmu_sro_dpt_l0_create(ctx, 0, &tmp);
        if (ret != RMI_SUCCESS) {
            return ret;
        }
        ret = psmmu_sro_activate(ctx, RMI_FEATURE_TRUE, sro);
    } else if (ret == RMI_ERROR_INPUT) {
        ret = psmmu_sro_activate(ctx, RMI_FEATURE_FALSE, sro);
    }

    return ret;
}
