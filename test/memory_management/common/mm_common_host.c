/*
 * Copyright (c) 2023, 2025-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "mm_common_host.h"
#include "val_host_rmi_sro.h"
#include "val_host_sro.h"

static uint64_t mm_release_destroyed_rec(mm_reclaim_sro_ts *sro)
{
    uint64_t ret;

    if ((sro == NULL) || (sro->rec == 0) || (sro->rec_destroyed != 0))
        return VAL_SUCCESS;

    val_host_update_destroy_granule_state(0, sro->rec, 0, 0,
                                          GRANULE_DELEGATED, GRANULE_REC, 0);
    ret = val_host_rmi_granule_range_undelegate(sro->rec, sro->rec + PAGE_SIZE);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "REC undelegation failed, rec=0x%lx ret=0x%lx\n", sro->rec, ret);
        return VAL_ERROR;
    }

    sro->rec_destroyed = 1U;
    sro->rec = 0;
    return VAL_SUCCESS;
}

static uint64_t mm_raw_realm_destroy(uint64_t rd)
{
    val_smc_param_ts args;

    args = val_host_rmi_realm_destroy_smc(rd);
    return val_host_sro_complete(&args);
}

uint64_t mm_prepare_rec_destroy_reclaim_sro(mm_reclaim_sro_ts *sro)
{
    if (sro == NULL)
        return VAL_ERROR;

    val_memset(sro, 0, sizeof(*sro));
    val_host_realm_params(&sro->realm);

    if (val_host_realm_setup(&sro->realm, false))
    {
        LOG(ERROR, "Realm setup failed\n");
        return VAL_ERROR;
    }

    sro->rec = sro->realm.rec[0];
    sro->args = val_host_rmi_rec_destroy_smc(sro->rec);
    if (RMI_STATUS(sro->args.x0) != RMI_INCOMPLETE)
    {
        LOG(TEST, "Skipping: RMI_REC_DESTROY did not start an SRO, ret=0x%lx\n",
            sro->args.x0);
        return VAL_SKIP_CHECK;
    }

    sro->handle = sro->args.x1;
    sro->sro_active = 1U;
    if (VAL_HOST_SRO_MEM_REQ(sro->args.x0) != RMI_OP_MEM_REQ_RECLAIM)
    {
        LOG(ERROR, "RMI_REC_DESTROY did not request reclaim, ret=0x%lx handle=0x%lx\n",
            sro->args.x0, sro->handle);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

uint64_t mm_reclaim_sro_once(mm_reclaim_sro_ts *sro,
                             uint64_t list_addr,
                             uint64_t list_count)
{
    uint64_t reclaim_count;

    if ((sro == NULL) || (sro->sro_active == 0) || (list_addr == 0) ||
        (list_count == 0) || (list_count > VAL_HOST_SRO_LIST_MAX_ENTRIES))
        return VAL_ERROR;

    val_memset((void *)list_addr, 0, PAGE_SIZE);
    sro->args = val_host_rmi_op_mem_reclaim_smc(sro->handle, list_addr, list_count);
    if (RMI_STATUS(sro->args.x0) != RMI_INCOMPLETE)
    {
        LOG(ERROR, "RMI_OP_MEM_RECLAIM failed, ret=0x%lx\n", sro->args.x0);
        return VAL_ERROR;
    }

    reclaim_count = sro->args.x1;
    if ((reclaim_count == 0) || (reclaim_count > list_count))
    {
        LOG(ERROR, "Invalid reclaim descriptor count: 0x%lx\n", reclaim_count);
        return VAL_ERROR;
    }

    if (val_host_sro_release_reclaimed(list_addr, reclaim_count))
    {
        LOG(ERROR, "Invalid or unreleasable reclaim descriptors\n");
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

uint64_t mm_complete_reclaim_sro(mm_reclaim_sro_ts *sro)
{
    uint64_t list_addr;
    uint64_t mem_req;
    uint64_t i;

    if ((sro == NULL) || (sro->sro_active == 0))
        return VAL_ERROR;

    list_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (list_addr == 0)
    {
        LOG(ERROR, "Failed to allocate reclaim list\n");
        return VAL_ERROR;
    }

    for (i = 0; i < VAL_HOST_SRO_MAX_ITERATIONS; i++)
    {
        mem_req = VAL_HOST_SRO_MEM_REQ(sro->args.x0);
        if (mem_req == RMI_OP_MEM_REQ_RECLAIM)
        {
            if (mm_reclaim_sro_once(sro, list_addr, VAL_HOST_SRO_LIST_MAX_ENTRIES))
            {
                val_host_mem_free((void *)list_addr);
                return VAL_ERROR;
            }
            continue;
        }

        if (mem_req == RMI_OP_MEM_REQ_NONE)
        {
            sro->args.x0 = val_host_sro_complete_with_handle(&sro->args, sro->handle);
            val_host_mem_free((void *)list_addr);
            if (sro->args.x0 == RMI_SUCCESS)
            {
                sro->sro_active = 0;
                if (mm_release_destroyed_rec(sro))
                    return VAL_ERROR;
            }

            return sro->args.x0;
        }

        LOG(ERROR, "Unexpected SRO memory request, ret=0x%lx\n", sro->args.x0);
        val_host_mem_free((void *)list_addr);
        return VAL_ERROR;
    }

    LOG(ERROR, "REC destroy reclaim completion exceeded max iterations\n");
    val_host_mem_free((void *)list_addr);
    return VAL_ERROR;
}

void mm_cleanup_reclaim_sro(mm_reclaim_sro_ts *sro)
{
    if (sro == NULL)
        return;

    if (sro->sro_active != 0)
    {
        sro->args.x0 = val_host_sro_complete_with_handle(&sro->args, sro->handle);
        if (sro->args.x0 == RMI_SUCCESS)
        {
            sro->sro_active = 0;
            (void)mm_release_destroyed_rec(sro);
        }
    }

    if (sro->realm.rd != 0)
        (void)val_host_realm_destroy(sro->realm.rd);
}

uint64_t mm_prepare_realm_create_sro(mm_realm_create_sro_ts *sro)
{
    val_host_realm_params_ts *params;

    if (sro == NULL)
        return VAL_ERROR;

    val_memset(sro, 0, sizeof(*sro));
    val_host_realm_params(&sro->realm);
    sro->realm.rec_count = 0;

    sro->realm.rd = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    sro->realm.rtt_l0_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    sro->params_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((sro->realm.rd == 0) || (sro->realm.rtt_l0_addr == 0) ||
        (sro->params_addr == 0))
        return VAL_ERROR;

    if (val_host_rmi_granule_range_delegate(sro->realm.rd,
                                            sro->realm.rd + PAGE_SIZE))
        return VAL_ERROR;
    sro->rd_delegated = 1;

    if (val_host_rmi_granule_range_delegate(sro->realm.rtt_l0_addr,
                                            sro->realm.rtt_l0_addr + PAGE_SIZE))
        return VAL_ERROR;
    sro->rtt_delegated = 1;

    params = (val_host_realm_params_ts *)sro->params_addr;
    val_memset(params, 0, PAGE_SIZE);
    params->flags0 = sro->realm.flags;
    params->flags1 = sro->realm.flags1;
    params->pmu_num_ctrs = sro->realm.pmu_num_ctrs;
    params->s2sz = sro->realm.s2sz;
    params->rtt_base = sro->realm.rtt_l0_addr;
    params->hash_algo = sro->realm.hash_algo;
    params->rtt_level_start = (int64_t)sro->realm.s2_starting_level;
    params->rtt_num_start = sro->realm.num_s2_sl_rtts;
    params->num_aux_planes = sro->realm.num_aux_planes;
    params->num_bps = sro->realm.num_bps + 1;
    params->num_wps = sro->realm.num_wps + 1;
    val_memcpy(&params->rpv, &sro->realm.rpv, sizeof(sro->realm.rpv));

    return VAL_SUCCESS;
}

void mm_cleanup_realm_create_sro(mm_realm_create_sro_ts *sro)
{
    if (sro == NULL)
        return;

    if (sro->realm_created != 0)
    {
        (void)val_host_rmi_realm_terminate(sro->realm.rd);
        if (mm_raw_realm_destroy(sro->realm.rd) == RMI_SUCCESS)
            sro->realm_created = 0;
    }

    if (sro->rtt_delegated != 0)
    {
        (void)val_host_rmi_granule_range_undelegate(sro->realm.rtt_l0_addr,
                                                    sro->realm.rtt_l0_addr +
                                                    PAGE_SIZE);
        sro->rtt_delegated = 0;
    }

    if (sro->rd_delegated != 0)
    {
        (void)val_host_rmi_granule_range_undelegate(sro->realm.rd,
                                                    sro->realm.rd + PAGE_SIZE);
        sro->rd_delegated = 0;
    }

    if (sro->params_addr != 0)
        val_host_mem_free((void *)sro->params_addr);
    if (sro->realm.rtt_l0_addr != 0)
        val_host_mem_free((void *)sro->realm.rtt_l0_addr);
    if (sro->realm.rd != 0)
        val_host_mem_free((void *)sro->realm.rd);
}

uint64_t mm_prepare_sro_donation_list(uint64_t donate_req,
                                      mm_sro_donation_ts *donation)
{
    uint64_t count = VAL_HOST_SRO_DONATE_REQ_COUNT(donate_req);
    uint64_t contig = VAL_HOST_SRO_DONATE_REQ_CONTIG(donate_req);
    uint64_t state = VAL_HOST_SRO_DONATE_REQ_STATE(donate_req);
    uint64_t size = VAL_HOST_SRO_DONATE_REQ_SIZE(donate_req);
    uint64_t alignment = PAGE_SIZE;
    uint64_t *list;

    if (donation == NULL)
        return VAL_ERROR;

    val_memset(donation, 0, sizeof(*donation));
    if ((size != RMI_PAGE_L3) || (count == 0) ||
        (count > VAL_HOST_SRO_DESC_MAX_COUNT))
        return VAL_ERROR;

    donation->mem_size = count * PAGE_SIZE;
    if ((donation->mem_size / PAGE_SIZE) != count)
        return VAL_ERROR;

    if (contig == RMI_OP_MEM_CONTIG)
        alignment = donation->mem_size;

    donation->list_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    donation->mem_base = (uint64_t)val_host_mem_alloc(alignment, donation->mem_size);
    if ((donation->list_addr == 0) || (donation->mem_base == 0))
        return VAL_ERROR;

    if (state == RMI_OP_MEM_DELEGATED)
    {
        if (val_host_rmi_granule_range_delegate(donation->mem_base,
                                                donation->mem_base +
                                                donation->mem_size))
            return VAL_ERROR;
        donation->mem_delegated = 1;
    }
    else if (state != RMI_OP_MEM_UNDELEGATED)
    {
        return VAL_ERROR;
    }

    list = (uint64_t *)donation->list_addr;
    val_memset(list, 0, PAGE_SIZE);
    list[0] = val_host_rmi_addr_range_desc(donation->mem_base, count, size, state);

    return VAL_SUCCESS;
}

void mm_release_sro_donation_list(mm_sro_donation_ts *donation)
{
    if (donation == NULL)
        return;

    if ((donation->mem_consumed == 0) && (donation->mem_delegated != 0))
        (void)val_host_rmi_granule_range_undelegate(donation->mem_base,
                                                    donation->mem_base +
                                                    donation->mem_size);

    if ((donation->mem_consumed == 0) && (donation->mem_base != 0))
        val_host_mem_free((void *)donation->mem_base);

    if (donation->list_addr != 0)
        val_host_mem_free((void *)donation->list_addr);
}

uint32_t val_host_init_ripas_delegate(val_host_realm_ts *realm,
                                            val_data_create_ts *data_create)
{
    uint64_t phys = data_create->target_pa;

    if (val_host_ripas_init(realm,
            data_create->ipa,
            data_create->ipa + data_create->size,
            VAL_RTT_MAX_LEVEL, data_create->rtt_alignment))
    {
        LOG(ERROR, "val_host_ripas_init failed, ipa=0x%x\n",
                data_create->ipa);
        return VAL_ERROR;
    }

    /* MAP image regions */
    if (val_host_rmi_granule_range_delegate(phys, phys + data_create->size))
    {
        LOG(ERROR, "Granule range delegation failed, PA=0x%x\n", phys);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}
