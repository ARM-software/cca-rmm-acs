/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_sro.h"

#define MAX_DONATION_ALLOCS 16U
#define SRO_ADDR_LIST_CAPACITY (PAGE_SIZE / sizeof(uint64_t))

struct donation_alloc {
    uint64_t list;
    uint64_t base;
    uint64_t size;
};

static uint64_t l0gpt_size_prep_sequence(void)
{
    uint64_t feature;
    uint64_t l0gptsz;

    if (val_host_rmi_features(RMI_FEATURE_REGISTER_1_INDEX, &feature))
        return VAL_TEST_PREP_SEQ_FAILED;

    l0gptsz = VAL_EXTRACT_BITS(feature, RMI_FEATURE_REGISTER_1_L0GPTSZ_START,
                               RMI_FEATURE_REGISTER_1_L0GPTSZ_END);

    return 1ULL << (RMI_L0GPTSZ_BASE_SHIFT + l0gptsz);
}

static uint64_t addr_range_desc(uint64_t addr, uint64_t count,
                                uint64_t state, uint64_t size)
{
    return size | (count << 2) | ADDR_ALIGN_DOWN(addr, PAGE_SIZE) |
           (state << 63);
}

static uint64_t addr_block_size(uint64_t size)
{
    uint64_t block_size = PAGE_SIZE;
    uint64_t entries_per_table = PAGE_SIZE / sizeof(uint64_t);

    if (size > RMI_BLOCK_L0)
        return 0;

    while (size-- != RMI_PAGE_L3)
    {
        if (block_size > (UINT64_MAX / entries_per_table))
            return 0;

        block_size *= entries_per_table;
    }

    return block_size;
}

static uint64_t is_power_of_two(uint64_t value)
{
    return (value != 0U) && ((value & (value - 1U)) == 0U);
}

static uint64_t build_addr_list(uint64_t list_addr, uint64_t base,
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
        if (entry_count >= SRO_ADDR_LIST_CAPACITY)
        {
            LOG(ERROR, "SRO donation address list is too small\n");
            return VAL_ERROR;
        }

        chunk = remaining;
        if (chunk > VAL_HOST_SRO_DESC_MAX_COUNT)
            chunk = VAL_HOST_SRO_DESC_MAX_COUNT;

        list[entry_count++] = addr_range_desc(addr, chunk, state, size);
        addr += chunk * block_size;
        remaining -= chunk;
    }

    *list_count = entry_count;
    return VAL_SUCCESS;
}

static uint64_t prepare_donation_list(uint64_t donate_req, uint64_t *list_addr,
                                      struct donation_alloc *allocs,
                                      uint32_t *alloc_count,
                                      uint64_t *list_count)
{
    uint64_t size = VAL_HOST_SRO_DONATE_REQ_SIZE(donate_req);
    uint64_t count = VAL_HOST_SRO_DONATE_REQ_COUNT(donate_req);
    uint64_t contig = VAL_HOST_SRO_DONATE_REQ_CONTIG(donate_req);
    uint64_t state = VAL_HOST_SRO_DONATE_REQ_STATE(donate_req);
    uint64_t alignment;
    uint64_t donated_base;
    uint64_t donated_size;
    uint64_t block_size;

    *list_addr = 0;
    *list_count = 0;

    block_size = addr_block_size(size);
    if ((block_size == 0) || (count == 0))
    {
        LOG(ERROR, "Invalid GPT L1 donate request, req=0x%lx\n", donate_req);
        return VAL_ERROR;
    }
    alignment = block_size;

    if ((state != RMI_OP_MEM_DELEGATED) && (state != RMI_OP_MEM_UNDELEGATED))
    {
        LOG(ERROR, "Invalid GPT L1 donate memory state, req=0x%lx\n", donate_req);
        return VAL_ERROR;
    }

    donated_size = count * block_size;
    if ((donated_size / block_size) != count)
    {
        LOG(ERROR, "GPT L1 donate size overflow, count=0x%lx\n", count);
        return VAL_ERROR;
    }

    if (contig == RMI_OP_MEM_CONTIG)
    {
        if (!is_power_of_two(donated_size))
        {
            LOG(ERROR, "Contiguous GPT L1 donate size is not power-of-two, size=0x%lx\n",
                donated_size);
            return VAL_ERROR;
        }
        alignment = donated_size;
    }

    if (*alloc_count == MAX_DONATION_ALLOCS)
    {
        LOG(ERROR, "Too many GPT L1 donation allocations\n");
        return VAL_ERROR;
    }

    *list_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    donated_base = (uint64_t)val_host_mem_alloc(alignment, donated_size);
    if ((*list_addr == 0) || (donated_base == 0))
    {
        LOG(ERROR, "Failed to allocate GPT L1 donation memory\n");
        if (*list_addr != 0)
            val_host_mem_free((void *)*list_addr);
        if (donated_base != 0)
            val_host_mem_free((void *)donated_base);
        *list_addr = 0;
        return VAL_ERROR;
    }

    if ((state == RMI_OP_MEM_DELEGATED) &&
        val_host_rmi_granule_range_delegate(donated_base, donated_base + donated_size))
    {
        LOG(ERROR, "Failed to delegate GPT L1 donation memory\n");
        val_host_mem_free((void *)*list_addr);
        val_host_mem_free((void *)donated_base);
        *list_addr = 0;
        return VAL_ERROR;
    }

    if (build_addr_list(*list_addr, donated_base, count, state, size,
                        block_size, list_count))
    {
        val_host_mem_free((void *)*list_addr);
        val_host_mem_free((void *)donated_base);
        *list_addr = 0;
        return VAL_ERROR;
    }

    allocs[*alloc_count].list = *list_addr;
    allocs[*alloc_count].base = donated_base;
    allocs[*alloc_count].size = donated_size;
    (*alloc_count)++;

    return VAL_SUCCESS;
}

static void free_donation_allocs(struct donation_alloc *allocs, uint32_t alloc_count)
{
    uint32_t i;

    for (i = 0; i < alloc_count; i++)
    {
        if (allocs[i].list != 0)
            val_host_mem_free((void *)allocs[i].list);
        if (allocs[i].base != 0)
            val_host_mem_free((void *)allocs[i].base);
    }
}

void mm_gpt_l1_create_sro_donate_host(void)
{
    val_smc_param_ts cmd_ret;
    struct donation_alloc allocs[MAX_DONATION_ALLOCS];
    uint64_t l0gpt_size;
    uint64_t probe_addr = 0;
    uint64_t gpt_addr;
    uint64_t list_addr = 0;
    uint64_t list_count;
    uint64_t sro_handle = 0;
    uint64_t saw_donate = 0;
    uint64_t donate_count = 0;
    uint32_t alloc_count = 0;
    uint32_t i;

    val_memset(allocs, 0, sizeof(allocs));

    l0gpt_size = l0gpt_size_prep_sequence();
    if (l0gpt_size == VAL_TEST_PREP_SEQ_FAILED)
    {
        LOG(ERROR, "Failed to read L0GPT size\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    probe_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (probe_addr == 0)
    {
        LOG(ERROR, "Failed to allocate GPT L1 probe granule\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return;
    }

    gpt_addr = ADDR_ALIGN_DOWN(probe_addr, l0gpt_size);
    cmd_ret = val_host_rmi_gpt_l1_create_smc(gpt_addr);
    if (RMI_STATUS(cmd_ret.x0) != RMI_INCOMPLETE)
    {
        if (cmd_ret.x0 == RMI_SUCCESS)
        {
            LOG(ERROR, "RMI_GPT_L1_CREATE completed without donation SRO\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        }
        else
        {
            LOG(TEST, "Skipping: RMI_GPT_L1_CREATE did not start an SRO, ret=0x%lx\n",
                cmd_ret.x0);
            val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        }
        goto free_probe;
    }

    sro_handle = cmd_ret.x1;

    for (i = 0; i < VAL_HOST_SRO_MAX_ITERATIONS; i++)
    {
        if (RMI_STATUS(cmd_ret.x0) == RMI_SUCCESS)
            break;

        if (RMI_STATUS(cmd_ret.x0) != RMI_INCOMPLETE)
        {
            LOG(ERROR, "RMI_GPT_L1_CREATE SRO failed, ret=0x%lx\n", cmd_ret.x0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto cancel_sro;
        }

        switch (VAL_HOST_SRO_MEM_REQ(cmd_ret.x0))
        {
            case RMI_OP_MEM_REQ_DONATE:
                if (prepare_donation_list(cmd_ret.x2, &list_addr, allocs,
                                          &alloc_count, &list_count))
                {
                    val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
                    goto cancel_sro;
                }

                cmd_ret = val_host_rmi_op_mem_donate_smc(sro_handle, list_addr,
                                                         list_count);
                list_addr = 0;
                saw_donate = 1;
                donate_count++;
                break;

            case RMI_OP_MEM_REQ_NONE:
                cmd_ret = val_host_rmi_op_continue_smc(sro_handle, RMI_CONTINUE_KEEP_GOING);
                break;

            default:
                LOG(ERROR, "RMI_GPT_L1_CREATE requested unexpected memory op, ret=0x%lx\n",
                    cmd_ret.x0);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
                goto cancel_sro;
        }
    }

    if (i == VAL_HOST_SRO_MAX_ITERATIONS)
    {
        LOG(ERROR, "RMI_GPT_L1_CREATE SRO exceeded max iterations\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto cancel_sro;
    }

    if (!saw_donate)
    {
        LOG(ERROR, "RMI_GPT_L1_CREATE completed without donation\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_l1;
    }

    LOG(TEST, "GPT L1 create donation count: 0x%lx\n", donate_count);
    val_set_status(RESULT_PASS(VAL_SUCCESS));
    goto destroy_l1;

cancel_sro:
    if (sro_handle != 0)
    {
        cmd_ret = val_host_rmi_op_cancel_smc(sro_handle);
        (void)cmd_ret;
    }
    goto free_allocs;

destroy_l1:
    if (val_host_rmi_gpt_l1_destroy(gpt_addr) != RMI_SUCCESS)
        LOG(TEST, "GPT L1 destroy cleanup did not complete\n");

free_allocs:
    free_donation_allocs(allocs, alloc_count);

free_probe:
    val_host_mem_free((void *)probe_addr);
}
