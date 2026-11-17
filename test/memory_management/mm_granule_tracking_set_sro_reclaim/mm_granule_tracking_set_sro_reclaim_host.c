/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_sro.h"

#define SIZE_2MB    (2UL * 1024UL * 1024UL)
#define SIZE_32MB   (32UL * 1024UL * 1024UL)
#define SIZE_512MB  (512UL * 1024UL * 1024UL)
#define SIZE_1GB    (1024UL * 1024UL * 1024UL)
#define SIZE_64GB   (64UL * SIZE_1GB)
#define SIZE_4TB    (4UL * 1024UL * SIZE_1GB)

static uint64_t tracking_region_size_from_rmi(uint64_t granule_size,
                                              uint64_t tracking_region_size)
{
    switch (granule_size)
    {
        case RMI_GRANULE_SIZE_4KB:
            if (tracking_region_size == 0)
                return SIZE_1GB;
            return (tracking_region_size == 1) ? SIZE_2MB : 0;

        case RMI_GRANULE_SIZE_16KB:
            if (tracking_region_size == 0)
                return SIZE_32MB;
            return (tracking_region_size == 1) ? SIZE_64GB : 0;

        case RMI_GRANULE_SIZE_64KB:
            if (tracking_region_size == 0)
                return SIZE_512MB;
            return (tracking_region_size == 1) ? SIZE_4TB : 0;

        default:
            return 0;
    }
}

static uint64_t addr_desc_count_hi(uint64_t granule_size)
{
    switch (granule_size)
    {
        case RMI_GRANULE_SIZE_4KB:
            return 11;
        case RMI_GRANULE_SIZE_16KB:
            return 13;
        case RMI_GRANULE_SIZE_64KB:
            return 15;
        default:
            return 0;
    }
}

static uint64_t addr_desc_addr_shift(uint64_t granule_size)
{
    switch (granule_size)
    {
        case RMI_GRANULE_SIZE_4KB:
            return PAGE_BITS_4K;
        case RMI_GRANULE_SIZE_16KB:
            return PAGE_BITS_16K;
        case RMI_GRANULE_SIZE_64KB:
            return PAGE_BITS_64K;
        default:
            return 0;
    }
}

static uint64_t rmi_granule_size_bytes(uint64_t granule_size)
{
    switch (granule_size)
    {
        case RMI_GRANULE_SIZE_4KB:
            return PAGE_SIZE_4K;
        case RMI_GRANULE_SIZE_16KB:
            return PAGE_SIZE_16K;
        case RMI_GRANULE_SIZE_64KB:
            return PAGE_SIZE_64K;
        default:
            return 0;
    }
}

static uint64_t addr_block_size(uint64_t granule_size, uint64_t size)
{
    uint64_t block_size = rmi_granule_size_bytes(granule_size);
    uint64_t entries_per_table;

    if ((block_size == 0) || (size > RMI_BLOCK_L0))
        return 0;

    entries_per_table = block_size / sizeof(uint64_t);
    while (size-- != RMI_PAGE_L3)
    {
        if (block_size > (UINT64_MAX / entries_per_table))
            return 0;

        block_size *= entries_per_table;
    }

    return block_size;
}

static uint64_t verify_and_track_reclaimed_desc(uint64_t desc, uint64_t granule_size)
{
    uint64_t count_hi = addr_desc_count_hi(granule_size);
    uint64_t addr_shift = addr_desc_addr_shift(granule_size);
    uint64_t granule_bytes = rmi_granule_size_bytes(granule_size);
    uint64_t block_size;
    uint64_t size;
    uint64_t count;
    uint64_t state;
    uint64_t addr;
    uint64_t i;
    uint64_t j;
    val_host_granule_ts *granule;

    if ((count_hi == 0) || (addr_shift == 0) || (granule_bytes == 0))
        return VAL_ERROR;

    size = VAL_EXTRACT_BITS(desc, 0, 1);
    count = VAL_EXTRACT_BITS(desc, 2, count_hi);
    state = VAL_EXTRACT_BITS(desc, 63, 63);
    addr = VAL_EXTRACT_BITS(desc, addr_shift, 51) << addr_shift;
    block_size = addr_block_size(granule_size, size);

    if ((block_size == 0) || (count == 0) || (state != RMI_OP_MEM_DELEGATED))
    {
        LOG(ERROR, "Invalid reclaimed descriptor: desc=0x%lx count=0x%lx "
            "state=0x%lx size=0x%lx\n", desc, count, state, size);
        return VAL_ERROR;
    }

    if ((count > ((UINT64_MAX - addr) / block_size)) ||
        ((addr + (count * block_size)) < addr))
    {
        LOG(ERROR, "Reclaimed descriptor range overflows: desc=0x%lx\n", desc);
        return VAL_ERROR;
    }

    for (i = 0; i < count; i++)
    {
        for (j = 0; j < (block_size / granule_bytes); j++)
        {
            uint64_t granule_addr = addr + (i * block_size) + (j * granule_bytes);

            granule = val_host_find_granule(granule_addr);
            if (granule == NULL)
            {
                val_host_add_granule(GRANULE_DELEGATED, granule_addr, NULL);
                granule = val_host_find_granule(granule_addr);
            }
            else
            {
                granule->state = GRANULE_DELEGATED;
            }

            if ((granule == NULL) || (granule->state != GRANULE_DELEGATED))
            {
                LOG(ERROR, "Reclaimed granule is not tracked as GRANULE_DELEGATED: 0x%lx\n",
                    granule_addr);
                return VAL_ERROR;
            }
        }
    }

    return VAL_SUCCESS;
}

static uint64_t reclaim_and_verify_tracking_sro(uint64_t sro_handle, uint64_t list_addr,
                                                uint64_t granule_size,
                                                uint64_t *total_reclaim_count)
{
    val_smc_param_ts cmd_ret;
    uint64_t reclaim_count;
    uint64_t *list = (uint64_t *)list_addr;
    uint64_t i;
    uint64_t iter;

    *total_reclaim_count = 0;

    for (iter = 0; iter < VAL_HOST_SRO_MAX_ITERATIONS; iter++)
    {
        val_memset(list, 0, PAGE_SIZE);
        cmd_ret = val_host_rmi_op_mem_reclaim_smc(sro_handle, list_addr,
                                                  VAL_HOST_SRO_LIST_MAX_ENTRIES);
        if (RMI_STATUS(cmd_ret.x0) != RMI_INCOMPLETE)
        {
            LOG(ERROR, "RMI_OP_MEM_RECLAIM failed, ret=0x%lx\n", cmd_ret.x0);
            return VAL_ERROR;
        }

        reclaim_count = cmd_ret.x1;
        if ((reclaim_count == 0) || (reclaim_count > VAL_HOST_SRO_LIST_MAX_ENTRIES))
        {
            LOG(ERROR, "Invalid reclaim descriptor count: 0x%lx\n", reclaim_count);
            return VAL_ERROR;
        }

        for (i = 0; i < reclaim_count; i++)
        {
            if (verify_and_track_reclaimed_desc(list[i], granule_size))
                return VAL_ERROR;
        }

        *total_reclaim_count += reclaim_count;
        if (VAL_HOST_SRO_MEM_REQ(cmd_ret.x0) == RMI_OP_MEM_REQ_NONE)
            break;

        if (VAL_HOST_SRO_MEM_REQ(cmd_ret.x0) != RMI_OP_MEM_REQ_RECLAIM)
        {
            LOG(ERROR, "RMI_OP_MEM_RECLAIM returned unexpected mem request, ret=0x%lx\n",
                cmd_ret.x0);
            return VAL_ERROR;
        }
    }

    if (iter == VAL_HOST_SRO_MAX_ITERATIONS)
    {
        LOG(ERROR, "RMI_GRANULE_TRACKING_SET reclaim exceeded max iterations\n");
        return VAL_ERROR;
    }

    cmd_ret.x0 = val_host_sro_complete_with_handle(&cmd_ret, sro_handle);
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_GRANULE_TRACKING_SET SRO completion failed, ret=0x%lx\n",
            cmd_ret.x0);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t restore_tracking_region(uint64_t tracking_addr,
                                        uint64_t tracking_region_size,
                                        uint64_t initial_state)
{
    val_smc_param_ts cmd_ret;

    cmd_ret = val_host_rmi_granule_tracking_set_smc(tracking_addr,
                                                    RMI_MEM_CATEGORY_CONVENTIONAL,
                                                    initial_state);
    if (RMI_STATUS(cmd_ret.x0) == RMI_INCOMPLETE)
        cmd_ret.x0 = val_host_sro_complete_tracking(&cmd_ret, tracking_addr,
                                                    tracking_region_size);

    return cmd_ret.x0;
}

void mm_granule_tracking_set_sro_reclaim_host(void)
{
    val_host_rmi_rmm_config_ts *cfg;
    val_smc_param_ts cmd_ret;
    uint64_t tracking_region_size;
    uint64_t tracking_addr = PLATFORM_TRACKING_TEST_REGION_BASE;
    uint64_t initial_state;
    uint64_t list_addr = 0;
    uint64_t sro_handle = 0;
    uint64_t reclaim_count;
    uint64_t restore_ret;

    cfg = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (cfg == NULL)
    {
        LOG(ERROR, "Failed to allocate RMM config structure\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    val_memset(cfg, 0, PAGE_SIZE);
    cmd_ret = val_host_rmi_rmm_config_get((uint64_t)cfg);
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_RMM_CONFIG_GET failed, ret=0x%lx\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto free_cfg;
    }

    tracking_region_size = tracking_region_size_from_rmi(cfg->rmi_granule_size,
                                                         cfg->tracking_region_size);
    if (tracking_region_size == 0)
    {
        LOG(ERROR, "Invalid RMI granule/tracking-region encoding: 0x%x/0x%x\n",
            cfg->rmi_granule_size, cfg->tracking_region_size);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto free_cfg;
    }

    cmd_ret = val_host_rmi_granule_tracking_get(tracking_addr,
                                                tracking_addr + tracking_region_size);
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(TEST, "Skipping: tracking region is not observable, ret=0x%lx\n", cmd_ret.x0);
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto free_cfg;
    }

    initial_state = cmd_ret.x2;
    if ((initial_state != RMI_TRACKING_FINE) &&
        (initial_state != RMI_TRACKING_COARSE) &&
        (initial_state != RMI_TRACKING_NONE))
    {
        LOG(TEST, "Skipping: unsupported tracking state 0x%lx\n", initial_state);
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto free_cfg;
    }

    if (initial_state != RMI_TRACKING_FINE)
    {
        cmd_ret = val_host_rmi_granule_tracking_set_smc(tracking_addr,
                                                        RMI_MEM_CATEGORY_CONVENTIONAL,
                                                        RMI_TRACKING_FINE);
        restore_ret = val_host_sro_complete_tracking(&cmd_ret, tracking_addr,
                                                     tracking_region_size);
        if (restore_ret != RMI_SUCCESS)
        {
            LOG(ERROR, "Failed to set test region to FINE, ret=0x%lx\n", restore_ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto free_cfg;
        }
    }

    cmd_ret = val_host_rmi_granule_tracking_set_smc(tracking_addr,
                                                    RMI_MEM_CATEGORY_CONVENTIONAL,
                                                    RMI_TRACKING_COARSE);
    if (cmd_ret.x0 == RMI_SUCCESS)
    {
        LOG(TEST, "Skipping: RMI_GRANULE_TRACKING_SET did not start an SRO, ret=0x%lx\n",
            cmd_ret.x0);
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto restore_region;
    }

    if (RMI_STATUS(cmd_ret.x0) != RMI_INCOMPLETE)
    {
        LOG(ERROR, "RMI_GRANULE_TRACKING_SET failed, ret=0x%lx\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto restore_region;
    }

    sro_handle = cmd_ret.x1;
    if (VAL_HOST_SRO_MEM_REQ(cmd_ret.x0) != RMI_OP_MEM_REQ_RECLAIM)
    {
        LOG(ERROR, "RMI_GRANULE_TRACKING_SET did not request reclaim, ret=0x%lx\n",
            cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto cancel_sro;
    }

    list_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (list_addr == 0)
    {
        LOG(ERROR, "Failed to allocate reclaim list\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto cancel_sro;
    }

    if (reclaim_and_verify_tracking_sro(sro_handle, list_addr, cfg->rmi_granule_size,
                                        &reclaim_count))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto cancel_sro;
    }

    LOG(TEST, "Reclaimed descriptor count: 0x%lx\n", reclaim_count);
    val_set_status(RESULT_PASS(VAL_SUCCESS));
    goto restore_region;

cancel_sro:
    cmd_ret = val_host_rmi_op_cancel_smc(sro_handle);
    if (cmd_ret.x0 != RMI_SUCCESS)
        LOG(ERROR, "RMI_OP_CANCEL failed, ret=0x%lx\n", cmd_ret.x0);

restore_region:
    restore_ret = restore_tracking_region(tracking_addr, tracking_region_size,
                                          initial_state);
    if (restore_ret != RMI_SUCCESS)
    {
        LOG(ERROR, "Failed to restore tracking region to state 0x%lx, ret=0x%lx\n",
            initial_state, restore_ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
    }

    if (list_addr != 0)
        val_host_mem_free((void *)list_addr);

free_cfg:
    val_host_mem_free(cfg);
}
