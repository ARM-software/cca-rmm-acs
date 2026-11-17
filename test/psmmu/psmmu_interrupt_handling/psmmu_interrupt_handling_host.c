/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "pal_psmmu.h"
#include "psmmu_sro_common.h"

#define PSMMU_IRQ_EVENT_BITS(x)     (((x) >> 1U) & 0x3U)
#define PSMMU_IRQ_PENDING(x)        ((x) & 0x1U)
#define PSMMU_IRQ_NOTIFY_MAX_LOOPS  (16U)

static uint32_t psmmu_irq_notify_until_clear(uint64_t psmmu, uint64_t irq_set,
                                             val_smc_param_ts *out)
{
    uint32_t loop = 0;

    do {
        *out = val_host_rmi_psmmu_irq_notify(psmmu, irq_set);
        if (out->x0 != RMI_SUCCESS) {
            LOG(ERROR, "PSMMU IRQ notify failed, ret=0x%lx.\n", out->x0);
            return VAL_ERROR;
        }

        if (PSMMU_IRQ_PENDING(out->x1) == 0U) {
            return VAL_SUCCESS;
        }

        loop++;
    } while (loop < PSMMU_IRQ_NOTIFY_MAX_LOOPS);

    LOG(ERROR, "PSMMU IRQ notify pending bit did not clear.\n");
    return VAL_ERROR;
}

static uint32_t psmmu_irq_expect_event(psmmu_sro_context_ts *ctx, uint64_t irq_set,
                                       uint64_t irq_num, uint64_t event,
                                       const char *name)
{
    val_smc_param_ts args;
    uint64_t reported_event;

    if (pal_psmmu_trigger_irq((uint32_t)irq_num) != 0) {
        LOG(TEST, "Failed to trigger PSMMU %s IRQ %lu.\n", name, irq_num);
        return VAL_SKIP_CHECK;
    }

    if (psmmu_irq_notify_until_clear(ctx->psmmu, irq_set, &args) != VAL_SUCCESS) {
        return VAL_ERROR;
    }

    reported_event = PSMMU_IRQ_EVENT_BITS(args.x1);
    if (reported_event == event) {
        return VAL_SUCCESS;
    }

    if (reported_event == RMI_PSMMU_IRQ_EVENT_NONE) {
        LOG(TEST, "No PSMMU %s IRQ event reported.\n", name);
        return VAL_SKIP_CHECK;
    }

    LOG(ERROR, "Unexpected PSMMU %s IRQ event 0x%lx.\n", name, reported_event);
    return VAL_ERROR;
}

void psmmu_interrupt_handling_host(void)
{
    psmmu_sro_context_ts ctx;
    psmmu_sro_result_ts sro;
    val_smc_param_ts args;
    uint32_t rc;
    uint64_t ret;

    rc = psmmu_sro_context_init_psmmu_only(&ctx);
    if (rc == VAL_SKIP_CHECK) {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }
    if (rc != VAL_SUCCESS) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    if ((ctx.info->flags & 0x3U) != RMI_IRQ_WIRED) {
        LOG(TEST, "PSMMU interrupt handling test requires wired IRQs.\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto cleanup;
    }

    ret = psmmu_sro_activate(&ctx, RMI_FEATURE_FALSE, &sro);
    if (ret == RMI_INCOMPLETE) {
        LOG(TEST, "PSMMU activation SRO could not be completed.\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto cleanup;
    }
    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "PSMMU activation failed, ret=0x%lx.\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto cleanup;
    }

    if (psmmu_irq_notify_until_clear(ctx.psmmu, 0, &args) != VAL_SUCCESS) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto cleanup;
    }
    if (PSMMU_IRQ_EVENT_BITS(args.x1) != RMI_PSMMU_IRQ_EVENT_NONE) {
        LOG(TEST, "Unexpected stale PSMMU IRQ event 0x%lx.\n",
            PSMMU_IRQ_EVENT_BITS(args.x1));
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto cleanup;
    }

    if (((ctx.info->flags >> 2U) & 0x1U) != 0U) {
        if (pal_psmmu_trigger_irq((uint32_t)ctx.info->cmdq_sync_intr_num) != 0) {
            LOG(TEST, "Failed to trigger PSMMU CMDQ-sync IRQ %u.\n",
                ctx.info->cmdq_sync_intr_num);
            val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
            goto cleanup;
        }
        if (psmmu_irq_notify_until_clear(ctx.psmmu,
                                         (1UL << RMI_PSMMU_IRQ_CMDQ),
                                         &args) != VAL_SUCCESS) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto cleanup;
        }
        if (PSMMU_IRQ_EVENT_BITS(args.x1) != RMI_PSMMU_IRQ_EVENT_NONE) {
            LOG(ERROR, "Unexpected PSMMU CMDQ-sync IRQ event 0x%lx.\n",
                PSMMU_IRQ_EVENT_BITS(args.x1));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto cleanup;
        }
    }

    rc = psmmu_irq_expect_event(&ctx, (1UL << RMI_PSMMU_IRQ_EVENTQ),
                                ctx.info->eventq_intr_num,
                                RMI_PSMMU_IRQ_EVENT_PSMMU, "EVENTQ");
    if (rc == VAL_SKIP_CHECK) {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto cleanup;
    }
    if (rc != VAL_SUCCESS) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto cleanup;
    }

    rc = psmmu_irq_expect_event(&ctx, (1UL << RMI_PSMMU_IRQ_PRIQ),
                                ctx.info->priq_intr_num,
                                RMI_PSMMU_IRQ_EVENT_PSMMU, "PRIQ");
    if (rc == VAL_SKIP_CHECK) {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto cleanup;
    }
    if (rc != VAL_SUCCESS) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto cleanup;
    }

    rc = psmmu_irq_expect_event(&ctx, (1UL << RMI_PSMMU_IRQ_GERROR),
                                ctx.info->gerror_intr_num,
                                RMI_PSMMU_IRQ_EVENT_ERROR, "GERROR");
    if (rc == VAL_SKIP_CHECK) {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto cleanup;
    }
    if (rc != VAL_SUCCESS) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto cleanup;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    psmmu_sro_context_cleanup(&ctx);
}
