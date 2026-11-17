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
                                    uint64_t state, uint64_t size,
                                    uint64_t block_size)
{
    return size | (count << 2) | ADDR_ALIGN_DOWN(addr, block_size) |
           (state << 63);
}

static uint32_t build_sro_addr_list(uint64_t list_addr, uint64_t base,
                                    uint64_t count, uint64_t state,
                                    uint64_t size, uint64_t block_size,
                                    uint64_t *list_count)
{
    uint64_t remaining = count;
    uint64_t addr = base;
    uint64_t entry_count = 0;
    uint64_t chunk;
    uint64_t *list = (uint64_t *)list_addr;

    val_memset(list, 0, PAGE_SIZE);

    while (remaining != 0)
    {
        if (entry_count == VAL_HOST_SRO_LIST_MAX_ENTRIES)
        {
            LOG(ERROR, "SRO address list is too small\n");
            return VAL_ERROR;
        }

        chunk = remaining;
        if (chunk > VAL_HOST_SRO_DESC_MAX_COUNT)
            chunk = VAL_HOST_SRO_DESC_MAX_COUNT;

        list[entry_count++] = sro_addr_range_desc(addr, chunk, state, size,
                                                  block_size);
        addr += chunk * block_size;
        remaining -= chunk;
    }

    *list_count = entry_count;
    return VAL_SUCCESS;
}

static uint32_t prepare_sro_donation(uint64_t donate_req, uint64_t *list_addr,
                                     uint64_t *list_count)
{
    uint64_t size = VAL_HOST_SRO_DONATE_REQ_SIZE(donate_req);
    uint64_t count = VAL_HOST_SRO_DONATE_REQ_COUNT(donate_req);
    uint64_t state = VAL_HOST_SRO_DONATE_REQ_STATE(donate_req);
    uint64_t block_size = sro_block_size(size);
    uint64_t donated_base;
    uint64_t donated_size;
    uint64_t max_list_blocks;

    *list_addr = 0;
    *list_count = 0;

    if ((block_size == 0) || (count == 0))
    {
        LOG(ERROR, "Unsupported SRO donation request, req=0x%lx\n", donate_req);
        return VAL_ERROR;
    }

    if ((state != RMI_OP_MEM_DELEGATED) && (state != RMI_OP_MEM_UNDELEGATED))
    {
        LOG(ERROR, "Invalid SRO donation state, req=0x%lx\n", donate_req);
        return VAL_ERROR;
    }

    max_list_blocks =
        (uint64_t)VAL_HOST_SRO_LIST_MAX_ENTRIES * VAL_HOST_SRO_DESC_MAX_COUNT;
    if (count > max_list_blocks)
    {
        LOG(ERROR, "SRO donation count too large, count=0x%lx\n", count);
        return VAL_ERROR;
    }

    donated_size = count * block_size;
    if ((donated_size / block_size) != count)
    {
        LOG(ERROR, "SRO donation size overflow, count=0x%lx\n", count);
        return VAL_ERROR;
    }

    *list_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    donated_base = (uint64_t)val_host_mem_alloc(block_size, donated_size);
    if ((*list_addr == 0) || (donated_base == 0))
    {
        LOG(ERROR, "Failed to allocate SRO donation memory\n");
        if (*list_addr != 0)
            val_host_mem_free((void *)*list_addr);
        if (donated_base != 0)
            val_host_mem_free((void *)donated_base);
        *list_addr = 0;
        return VAL_ERROR;
    }

    if ((state == RMI_OP_MEM_DELEGATED) &&
        val_host_rmi_granule_range_delegate(donated_base,
                                            donated_base + donated_size))
    {
        LOG(ERROR, "Failed to delegate SRO donation memory\n");
        val_host_mem_free((void *)*list_addr);
        val_host_mem_free((void *)donated_base);
        *list_addr = 0;
        return VAL_ERROR;
    }

    if (build_sro_addr_list(*list_addr, donated_base, count, state, size,
                            block_size, list_count))
    {
        val_host_mem_free((void *)*list_addr);
        val_host_mem_free((void *)donated_base);
        *list_addr = 0;
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint32_t complete_rec_create_sro(val_smc_param_ts *cmd_ret)
{
    uint64_t handle = cmd_ret->x1;
    uint64_t status;
    uint64_t list_addr;
    uint64_t list_count;
    uint64_t i;

    for (i = 0; i < VAL_HOST_SRO_MAX_ITERATIONS; i++)
    {
        status = RMI_STATUS(cmd_ret->x0);

        if (status == RMI_BUSY)
        {
            *cmd_ret = val_host_rmi_op_continue_smc(handle, RMI_CONTINUE_KEEP_GOING);
            continue;
        }

        if (status == RMI_SUCCESS)
            return VAL_SUCCESS;

        if (status != RMI_INCOMPLETE)
        {
            LOG(ERROR, "SRO command failed, ret=0x%lx\n", cmd_ret->x0);
            return VAL_ERROR;
        }

        if (VAL_HOST_SRO_MEM_REQ(cmd_ret->x0) == RMI_OP_MEM_REQ_DONATE)
        {
            if (prepare_sro_donation(cmd_ret->x2, &list_addr, &list_count))
                return VAL_ERROR;

            *cmd_ret = val_host_rmi_op_mem_donate_smc(handle, list_addr, list_count);
            val_host_mem_free((void *)list_addr);
            continue;
        }

        if (VAL_HOST_SRO_MEM_REQ(cmd_ret->x0) == RMI_OP_MEM_REQ_NONE)
        {
            *cmd_ret = val_host_rmi_op_continue_smc(handle, RMI_CONTINUE_KEEP_GOING);
            continue;
        }

        LOG(ERROR, "Unexpected SRO memory request, ret=0x%lx\n", cmd_ret->x0);
        return VAL_ERROR;
    }

    LOG(ERROR, "SRO completion exceeded max iterations\n");
    return VAL_ERROR;
}

void mm_sro_intermediate_continue_host(void)
{
    val_host_realm_ts realm;
    val_host_rec_params_ts *rec_params = NULL;
    val_host_rec_create_flags_ts rec_create_flags;
    val_smc_param_ts cmd_ret;
    val_smc_param_ts rec_destroy_ret;
    uint64_t rec = 0;
    uint64_t run = 0;
    bool realm_created = false;
    bool rec_delegated = false;
    bool rec_created = false;

    val_memset(&realm, 0, sizeof(realm));
    val_host_realm_params(&realm);
    realm.rec_count = 0;

    if (val_host_realm_create(&realm))
    {
        LOG(ERROR, "Realm create failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto cleanup;
    }
    realm_created = true;

    rec_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    run = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    rec = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((rec_params == NULL) || (run == 0) || (rec == 0))
    {
        LOG(ERROR, "Failed to allocate REC resources\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto cleanup;
    }

    val_memset(rec_params, 0, PAGE_SIZE);
    val_memset((void *)run, 0, PAGE_SIZE);
    val_memset(&rec_create_flags, 0, sizeof(rec_create_flags));
    rec_create_flags.runnable = RMI_RUNNABLE;
    val_memcpy(&rec_params->flags, &rec_create_flags, sizeof(rec_create_flags));
    rec_params->pc = VAL_PLANE0_IMAGE_BASE_IPA;
    rec_params->mpidr = 0;

    if (val_host_rmi_granule_range_delegate(rec, rec + PAGE_SIZE))
    {
        LOG(ERROR, "REC granule delegation failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto cleanup;
    }
    rec_delegated = true;

    cmd_ret = val_host_rmi_rec_create_smc(realm.rd, rec, (uint64_t)rec_params);
    if (RMI_STATUS(cmd_ret.x0) != RMI_INCOMPLETE)
    {
        LOG(ERROR, "RMI_REC_CREATE did not create an intermediate SRO, ret=0x%lx\n",
            cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto cleanup;
    }

    if (VAL_HOST_SRO_MEM_REQ(cmd_ret.x0) != RMI_OP_MEM_REQ_DONATE)
    {
        LOG(ERROR, "RMI_REC_CREATE did not request SRO donation, ret=0x%lx\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto cancel_sro;
    }

    rec_destroy_ret = val_host_rmi_rec_destroy_smc(rec);
    if (RMI_STATUS(rec_destroy_ret.x0) != RMI_ERROR_INPUT)
    {
        LOG(ERROR, "Intermediate REC destroy returned unexpected status, ret=0x%lx\n",
            rec_destroy_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto cancel_sro;
    }

    if (complete_rec_create_sro(&cmd_ret))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto cancel_sro;
    }

    val_host_update_granule_state(realm.rd, GRANULE_REC, rec, 0, 0, 0);
    rec_created = true;

    if (val_host_rmi_rec_destroy(rec))
    {
        LOG(ERROR, "RMI_REC_DESTROY failed after SRO completion\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto cleanup;
    }
    rec_created = false;

    if (val_host_rmi_granule_range_undelegate(rec, rec + PAGE_SIZE))
    {
        LOG(ERROR, "REC granule undelegation failed after REC_DESTROY\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto cleanup;
    }
    rec_delegated = false;

    val_set_status(RESULT_PASS(VAL_SUCCESS));
    goto cleanup;

cancel_sro:
    (void)val_host_rmi_op_cancel_smc(cmd_ret.x1);

cleanup:
    if (rec_created)
        (void)val_host_rmi_rec_destroy(rec);

    if (rec_delegated)
        (void)val_host_rmi_granule_range_undelegate(rec, rec + PAGE_SIZE);

    if (realm_created)
        (void)val_host_realm_destroy(realm.rd);

    if (rec_params != NULL)
        val_host_mem_free(rec_params);
    if (run != 0)
        val_host_mem_free((void *)run);
    if (rec != 0)
        val_host_mem_free((void *)rec);
}
