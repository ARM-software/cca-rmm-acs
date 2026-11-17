/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_realm.h"
#include "val_host_rmi.h"
#include "val_host_rmi_sro.h"
#include "val_host_sro.h"

struct rec_create_sro {
    val_host_realm_ts realm;
    val_smc_param_ts sro_args;
    uint64_t handle;
    uint64_t donate_req;
    uint64_t rec;
    bool realm_created;
    bool rec_delegated;
    bool rec_created;
};

struct donation_list {
    uint64_t list_alloc;
    uint64_t list_addr;
    uint64_t list_count;
    uint64_t mem_base;
    uint64_t mem_size;
    bool mem_delegated;
};

static void release_donation_list(struct donation_list *donation);

static uint64_t sro_block_size(uint64_t size)
{
    uint64_t block_size = PAGE_SIZE;
    uint64_t block_scale = 1UL << (VAL_HOST_SRO_ADDR_SHIFT - 3U);
    uint64_t i;

    if (size > RMI_BLOCK_L0)
        return 0;

    for (i = 0; i < size; i++)
    {
        if ((block_size * block_scale) / block_scale != block_size)
            return 0;
        block_size *= block_scale;
    }

    return block_size;
}

static uint64_t sro_addr_range_desc(uint64_t addr, uint64_t count,
                                    uint64_t state, uint64_t size)
{
    return size | (count << 2) | ADDR_ALIGN_DOWN(addr, PAGE_SIZE) |
           (state << 63);
}

static uint64_t setup_rec_create_sro(struct rec_create_sro *sro)
{
    val_host_rec_params_ts *params = NULL;

    val_memset(sro, 0, sizeof(*sro));
    val_host_realm_params(&sro->realm);
    sro->realm.rec_count = 0;

    if (val_host_realm_create(&sro->realm))
    {
        LOG(ERROR, "Realm create failed\n");
        return VAL_ERROR;
    }
    sro->realm_created = true;

    sro->rec = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((sro->rec == 0) || (params == NULL))
    {
        LOG(ERROR, "Failed to allocate REC create resources\n");
        goto err;
    }

    if (val_host_rmi_granule_range_delegate(sro->rec, sro->rec + PAGE_SIZE))
    {
        LOG(ERROR, "REC granule delegation failed\n");
        goto err;
    }
    sro->rec_delegated = true;

    val_memset(params, 0, PAGE_SIZE);
    params->pc = 0;
    params->flags = RMI_RUNNABLE;
    params->mpidr = 0;
    params->num_aux = 0;

    sro->sro_args = val_host_rmi_rec_create_smc(sro->realm.rd, sro->rec,
                                                (uint64_t)params);
    if (RMI_STATUS(sro->sro_args.x0) != RMI_INCOMPLETE)
    {
        LOG(ERROR, "RMI_REC_CREATE did not start an SRO, ret=0x%lx\n",
            sro->sro_args.x0);
        goto err;
    }

    if (VAL_HOST_SRO_MEM_REQ(sro->sro_args.x0) != RMI_OP_MEM_REQ_DONATE)
    {
        LOG(ERROR, "RMI_REC_CREATE did not request donation, ret=0x%lx\n",
            sro->sro_args.x0);
        goto err;
    }

    sro->handle = sro->sro_args.x1;
    sro->donate_req = sro->sro_args.x2;
    val_host_mem_free(params);
    return VAL_SUCCESS;

err:
    if (params != NULL)
        val_host_mem_free(params);
    return VAL_ERROR;
}

static uint64_t prepare_donation_list(struct donation_list *donation,
                                      uint64_t donate_req,
                                      uint64_t count_delta,
                                      bool delegate)
{
    uint64_t count = VAL_HOST_SRO_DONATE_REQ_COUNT(donate_req) + count_delta;
    uint64_t state = VAL_HOST_SRO_DONATE_REQ_STATE(donate_req);
    uint64_t size = VAL_HOST_SRO_DONATE_REQ_SIZE(donate_req);
    uint64_t contig = VAL_HOST_SRO_DONATE_REQ_CONTIG(donate_req);
    uint64_t block_size = sro_block_size(size);
    uint64_t alignment;
    uint64_t *list;

    val_memset(donation, 0, sizeof(*donation));

    if ((count == 0) || (count > VAL_HOST_SRO_DESC_MAX_COUNT) ||
        (block_size == 0))
        return VAL_ERROR;

    donation->mem_size = count * block_size;
    if ((donation->mem_size / block_size) != count)
        return VAL_ERROR;

    alignment = (contig == RMI_OP_MEM_CONTIG) ? donation->mem_size : block_size;
    donation->list_alloc = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    donation->mem_base = (uint64_t)val_host_mem_alloc(alignment, donation->mem_size);
    if ((donation->list_alloc == 0) || (donation->mem_base == 0))
    {
        LOG(ERROR, "Failed to allocate donation list resources\n");
        release_donation_list(donation);
        return VAL_ERROR;
    }

    if (delegate &&
        val_host_rmi_granule_range_delegate(donation->mem_base,
                                            donation->mem_base + donation->mem_size))
    {
        LOG(ERROR, "Failed to delegate donation memory\n");
        release_donation_list(donation);
        return VAL_ERROR;
    }
    donation->mem_delegated = delegate;

    list = (uint64_t *)donation->list_alloc;
    val_memset(list, 0, PAGE_SIZE);
    list[0] = sro_addr_range_desc(donation->mem_base, count, state, size);

    donation->list_addr = donation->list_alloc;
    donation->list_count = 1;
    return VAL_SUCCESS;
}

static void release_donation_list(struct donation_list *donation)
{
    if (donation->mem_delegated)
    {
        if (val_host_rmi_granule_range_undelegate(donation->mem_base,
                                                  donation->mem_base +
                                                  donation->mem_size))
        {
            LOG(ERROR, "Failed to undelegate rejected donation memory\n");
        }
    }

    if (donation->mem_base != 0)
        val_host_mem_free((void *)donation->mem_base);
    if (donation->list_alloc != 0)
        val_host_mem_free((void *)donation->list_alloc);

    val_memset(donation, 0, sizeof(*donation));
}

static void cleanup_rec_create_sro(struct rec_create_sro *sro)
{
    if (sro->rec_created)
    {
        if (val_host_rmi_rec_destroy(sro->rec) == RMI_SUCCESS)
            sro->rec_created = false;
    }

    if (sro->rec_delegated)
    {
        if (val_host_rmi_granule_range_undelegate(sro->rec, sro->rec + PAGE_SIZE) ==
            RMI_SUCCESS)
            sro->rec_delegated = false;
    }

    if (sro->realm_created)
    {
        (void)val_host_realm_destroy(sro->realm.rd);
        sro->realm_created = false;
    }

    if (sro->rec != 0)
    {
        val_host_mem_free((void *)sro->rec);
        sro->rec = 0;
    }
}

void mm_sro_invalid_donation_host(void)
{
    struct rec_create_sro sro;
    struct donation_list donation;
    val_smc_param_ts cmd_ret;
    bool delegate;

    val_memset(&donation, 0, sizeof(donation));

    if (setup_rec_create_sro(&sro))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto cleanup;
    }

    delegate = VAL_HOST_SRO_DONATE_REQ_STATE(sro.donate_req) ==
               RMI_OP_MEM_DELEGATED;

    if (prepare_donation_list(&donation, sro.donate_req, 1, delegate))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto cleanup;
    }

    cmd_ret = val_host_rmi_op_mem_donate_smc(sro.handle, donation.list_addr,
                                             donation.list_count);
    if (RMI_STATUS(cmd_ret.x0) != RMI_ERROR_INPUT)
    {
        LOG(ERROR, "Invalid memory donation was not rejected, ret=0x%lx\n",
            cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto cleanup;
    }

    LOG(TEST, "Invalid memory donation rejected, ret=0x%lx\n", cmd_ret.x0);

    cmd_ret = val_host_rmi_op_continue_smc(sro.handle, RMI_CONTINUE_KEEP_GOING);
    if ((RMI_STATUS(cmd_ret.x0) == RMI_BUSY) ||
        (RMI_STATUS(cmd_ret.x0) == RMI_INCOMPLETE) ||
        (RMI_STATUS(cmd_ret.x0) == RMI_SUCCESS))
    {
        LOG(ERROR, "RMI_OP_CONTINUE did not complete failed SRO, ret=0x%lx\n",
            cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto cleanup;
    }

    cmd_ret = sro.sro_args;
    if (val_host_sro_complete(&cmd_ret) != RMI_SUCCESS)
    {
        LOG(ERROR, "REC_CREATE SRO completion failed after rejected donation\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto cleanup;
    }

    val_host_update_granule_state(sro.realm.rd, GRANULE_REC, sro.rec, 0, 0, 0);
    sro.rec_created = true;
    sro.rec_delegated = false;

    if (val_host_rmi_rec_destroy(sro.rec) != RMI_SUCCESS)
    {
        LOG(ERROR, "REC destroy failed after valid SRO completion\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto cleanup;
    }
    sro.rec_created = false;
    sro.rec_delegated = true;

    if (val_host_rmi_granule_range_undelegate(sro.rec, sro.rec + PAGE_SIZE))
    {
        LOG(ERROR, "REC granule undelegation failed after REC_DESTROY\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto cleanup;
    }
    sro.rec_delegated = false;

    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    release_donation_list(&donation);
    cleanup_rec_create_sro(&sro);
}
