/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_sro.h"
#include "val_host_alloc.h"
#include "val_host_realm.h"
#include "val_host_rmi.h"
#include "val_libc.h"

typedef void *(*val_host_sro_alloc_fn)(size_t alignment, size_t size);

/**
 *   @brief    Creates an SRO address range descriptor
 *   @param    addr     - Base address of the range
 *   @param    count    - Number of granules in the range
 *   @param    state    - Granule state encoded in the descriptor
 *   @return   Encoded address range descriptor
**/
static uint64_t val_host_sro_addr_range_desc(uint64_t addr, uint64_t count, uint64_t state)
{
    return RMI_PAGE_L3 |
           (count << 2) |
           ADDR_ALIGN_DOWN(addr, PAGE_SIZE) |
           (state << 62);
}

static uint32_t val_host_sro_is_power_of_two(uint64_t value)
{
    return (value != 0U) && ((value & (value - 1U)) == 0U);
}

/**
 *   @brief    Continues an SRO that does not request memory
 *   @param    args    - RMI output registers for the in-progress SRO
 *   @return   SUCCESS/FAILURE
**/
static uint32_t val_host_sro_continue_no_mem(val_smc_param_ts *args)
{
    uint64_t handle = args->x1;
    uint64_t status;
    uint32_t i;

    for (i = 0; i < VAL_HOST_SRO_MAX_ITERATIONS; i++)
    {
        status = RMI_STATUS(args->x0);

        if (status == RMI_BUSY)
        {
            *args = val_host_rmi_op_continue_smc(handle, RMI_CONTINUE_KEEP_GOING);
            continue;
        }

        if (status == RMI_SUCCESS)
            return VAL_SUCCESS;

        if (status != RMI_INCOMPLETE)
        {
            LOG(ERROR, "SRO continue failed, ret=0x%x\n", args->x0);
            return VAL_ERROR;
        }

        if (VAL_HOST_SRO_MEM_REQ(args->x0) != RMI_OP_MEM_REQ_NONE)
            return VAL_ERROR;

        *args = val_host_rmi_op_continue_smc(handle, RMI_CONTINUE_KEEP_GOING);
    }

    LOG(ERROR, "SRO continue exceeded max iterations\n");
    return VAL_ERROR;
}

/**
 *   @brief    Delegates a granule range and handles no-memory SRO continuation
 *   @param    base     - Base address of the range to delegate
 *   @param    count    - Number of granules to delegate
 *   @param    add_to_mem_track - Whether to track granules for test cleanup
 *   @return   SUCCESS/FAILURE
**/
static uint32_t val_host_sro_delegate_range(uint64_t base, uint64_t count,
                                            bool add_to_mem_track)
{
    val_smc_param_ts args;
    uint64_t top;
    uint64_t next = base;
    uint64_t status;
    uint32_t i;

    if ((base == 0) || (count == 0))
        return VAL_ERROR;

    if (count > ((((uint64_t)-1) - base) / PAGE_SIZE))
        return VAL_ERROR;

    top = base + (count * PAGE_SIZE);

    for (i = 0; i < VAL_HOST_SRO_MAX_ITERATIONS && next < top; i++)
    {
        args = val_host_rmi_granule_range_delegate_smc(next, top);
        status = RMI_STATUS(args.x0);

        if (status == RMI_BUSY)
            continue;

        if (status == RMI_INCOMPLETE)
        {
            if (val_host_sro_continue_no_mem(&args))
            {
                LOG(ERROR, "Failed to continue GRANULE_RANGE_DELEGATE SRO\n");
                return VAL_ERROR;
            }

            status = RMI_STATUS(args.x0);
        }

        if (status != RMI_SUCCESS)
        {
            LOG(ERROR, "GRANULE_RANGE_DELEGATE failed, ret=0x%x\n", args.x0);
            return VAL_ERROR;
        }

        if ((args.x1 <= next) || (args.x1 > top))
        {
            LOG(ERROR, "GRANULE_RANGE_DELEGATE made invalid progress, out_top=0x%x\n",
                args.x1);
            return VAL_ERROR;
        }

        next = args.x1;
    }

    if (next != top)
    {
        LOG(ERROR, "GRANULE_RANGE_DELEGATE exceeded max iterations\n");
        return VAL_ERROR;
    }

    if (add_to_mem_track)
    {
        for (i = 0; i < count; i++)
            val_host_add_granule(GRANULE_DELEGATED, base + (i * PAGE_SIZE), NULL);
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    Builds an SRO memory address list
 *   @param    list_addr     - Address of the list buffer
 *   @param    base          - Base address of the memory range
 *   @param    count         - Number of granules in the memory range
 *   @param    state         - Granule state encoded in each descriptor
 *   @param    list_count    - Pointer to store the number of descriptors
 *   @return   SUCCESS/FAILURE
**/
static uint32_t val_host_sro_build_addr_list(uint64_t list_addr, uint64_t base,
                                             uint64_t count, uint64_t state,
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

        list[entry_count++] = val_host_sro_addr_range_desc(addr, chunk, state);
        addr += chunk * PAGE_SIZE;
        remaining -= chunk;
    }

    *list_count = entry_count;
    return VAL_SUCCESS;
}

/**
 *   @brief    Allocates and prepares memory requested by an SRO donation
 *   @param    donate_req    - RMI donation request descriptor
 *   @param    list_addr     - Pointer to store the address-list buffer
 *   @param    list_count    - Pointer to store the number of descriptors
 *   @return   SUCCESS/FAILURE
**/
static uint32_t val_host_sro_prepare_donation(uint64_t donate_req, uint64_t *list_addr,
                                              uint64_t *list_count,
                                              uint64_t region_base, uint64_t region_size,
                                              val_host_sro_alloc_fn alloc,
                                              bool add_to_mem_track)
{
    uint64_t size = VAL_HOST_SRO_DONATE_REQ_SIZE(donate_req);
    uint64_t count = VAL_HOST_SRO_DONATE_REQ_COUNT(donate_req);
    uint64_t contig = VAL_HOST_SRO_DONATE_REQ_CONTIG(donate_req);
    uint64_t state = VAL_HOST_SRO_DONATE_REQ_STATE(donate_req);
    uint64_t alignment = PAGE_SIZE;
    uint64_t donated_base;
    uint64_t donated_size;
    uint64_t donated_top;
    uint64_t region_top = 0U;
    uint64_t max_list_granules;

    *list_addr = 0;
    *list_count = 0;

    if ((size != RMI_PAGE_L3) || (count == 0))
    {
        LOG(ERROR, "Unsupported SRO donation request, req=0x%x\n", donate_req);
        return VAL_ERROR;
    }

    if ((state != RMI_OP_MEM_DELEGATED) && (state != RMI_OP_MEM_UNDELEGATED) &&
        (state != RMI_OP_MEM_CONDITIONAL))
    {
        LOG(ERROR, "Invalid SRO donation state, req=0x%x\n", donate_req);
        return VAL_ERROR;
    }

    if ((state == RMI_OP_MEM_CONDITIONAL) && (region_size != 0U))
    {
        if (region_base > UINT64_MAX - region_size)
        {
            LOG(ERROR, "Tracking region address range overflows\n");
            return VAL_ERROR;
        }
        region_top = region_base + region_size;
    }

    max_list_granules =
        (uint64_t)VAL_HOST_SRO_LIST_MAX_ENTRIES * VAL_HOST_SRO_DESC_MAX_COUNT;
    if (count > max_list_granules)
    {
        LOG(ERROR, "SRO donation count too large, count=0x%x\n", count);
        return VAL_ERROR;
    }

    donated_size = count * PAGE_SIZE;
    if ((donated_size / PAGE_SIZE) != count)
    {
        LOG(ERROR, "SRO donation size overflow, count=0x%x\n", count);
        return VAL_ERROR;
    }

    if (contig == RMI_OP_MEM_CONTIG)
    {
        if (!val_host_sro_is_power_of_two(donated_size))
        {
            LOG(ERROR, "Contiguous SRO donation size is not power-of-two, size=0x%x\n",
                donated_size);
            return VAL_ERROR;
        }
        alignment = donated_size;
    }

    *list_addr = (uint64_t)alloc(PAGE_SIZE, PAGE_SIZE);
    donated_base = (uint64_t)alloc(alignment, donated_size);
    if ((*list_addr == 0) || (donated_base == 0))
    {
        LOG(ERROR, "Failed to allocate SRO donation memory\n");
        if (*list_addr != 0)
            val_host_mem_free((void *)*list_addr);
        if (donated_base != 0)
            val_host_mem_free((void *)donated_base);
        return VAL_ERROR;
    }

    donated_top = donated_base + donated_size;
    if (donated_top < donated_base)
    {
        LOG(ERROR, "SRO donation range overflow, base=0x%x size=0x%x\n",
            donated_base, donated_size);
        val_host_mem_free((void *)*list_addr);
        val_host_mem_free((void *)donated_base);
        return VAL_ERROR;
    }

    /* Conditional memory inside the tracking region remains undelegated. */
    if (state == RMI_OP_MEM_CONDITIONAL)
    {
        if ((region_size != 0U) &&
            (donated_base < region_top) &&
            (donated_top > region_base) &&
            ((donated_base < region_base) || (donated_top > region_top)))
        {
            LOG(ERROR, "Conditional SRO donation [0x%lx, 0x%lx) crosses "
                "tracking region [0x%lx, 0x%lx)\n",
                donated_base, donated_top, region_base, region_top);
            val_host_mem_free((void *)*list_addr);
            val_host_mem_free((void *)donated_base);
            return VAL_ERROR;
        }

        if ((region_size != 0U) &&
            (donated_base >= region_base) &&
            (donated_top <= region_top))
            state = RMI_OP_MEM_UNDELEGATED;
        else
            state = RMI_OP_MEM_DELEGATED;
    }

    if ((state == RMI_OP_MEM_DELEGATED) &&
        val_host_sro_delegate_range(donated_base, count, add_to_mem_track))
    {
        val_host_mem_free((void *)*list_addr);
        val_host_mem_free((void *)donated_base);
        return VAL_ERROR;
    }

    if (val_host_sro_build_addr_list(*list_addr, donated_base, count, state, list_count))
    {
        val_host_mem_free((void *)*list_addr);
        val_host_mem_free((void *)donated_base);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    Donates memory requested by an in-progress SRO
 *   @param    handle    - SRO operation handle
 *   @param    args      - RMI output registers for the in-progress SRO
 *   @return   SUCCESS/FAILURE
**/
static uint32_t val_host_sro_donate(uint64_t handle, val_smc_param_ts *args,
                                    uint64_t region_base, uint64_t region_size,
                                    val_host_sro_alloc_fn alloc,
                                    bool add_to_mem_track)
{
    uint64_t donate_req = args->x2;
    uint64_t list_addr;
    uint64_t list_count;
    uint64_t status;
    uint32_t i;
    uint32_t ret = VAL_ERROR;

    if (val_host_sro_prepare_donation(donate_req, &list_addr, &list_count,
                                      region_base, region_size, alloc,
                                      add_to_mem_track))
        return VAL_ERROR;

    for (i = 0; i < VAL_HOST_SRO_MAX_ITERATIONS; i++)
    {
        *args = val_host_rmi_op_mem_donate_smc(handle, list_addr, list_count);
        status = RMI_STATUS(args->x0);

        if (status == RMI_BUSY)
            continue;

        if (status == RMI_INCOMPLETE)
        {
            ret = VAL_SUCCESS;
            break;
        }

        if (status == RMI_SUCCESS)
        {
            ret = VAL_SUCCESS;
            break;
        }

        *args = val_host_rmi_op_continue_smc(handle, RMI_CONTINUE_KEEP_GOING);
        status = RMI_STATUS(args->x0);
        if ((status == RMI_SUCCESS) || (status == RMI_INCOMPLETE) || (status == RMI_BUSY))
        {
            ret = VAL_SUCCESS;
            break;
        }

        LOG(ERROR, "SRO continue after donate failed, ret=0x%x\n", args->x0);
        break;
    }

    val_host_mem_free((void *)list_addr);

    if (ret != VAL_SUCCESS)
        LOG(ERROR, "RMI_OP_MEM_DONATE exceeded max iterations\n");

    return ret;
}

/**
 *   @brief    Releases memory returned by RMI_OP_MEM_RECLAIM
 *   @param    list_addr     - Address of the populated RMI Address Range List
 *   @param    list_count    - Number of valid descriptors in the list
 *   @return   SUCCESS/FAILURE
**/
uint32_t val_host_sro_release_reclaimed(uint64_t list_addr, uint64_t list_count)
{
    uint64_t *list = (uint64_t *)list_addr;
    uint64_t block_shift;
    uint64_t block_size;
    uint64_t state;
    uint64_t size;
    uint64_t base;
    uint64_t count;
    uint64_t range_size;
    uint64_t top;
    uint64_t i;

    if ((list_count == 0U) || (list_count > VAL_HOST_SRO_LIST_MAX_ENTRIES))
        return VAL_ERROR;

    for (i = 0; i < list_count; i++)
    {
        if (VAL_EXTRACT_BITS(list[i], 52, 61) != 0U)
            return VAL_ERROR;

        size = VAL_HOST_RMI_ADDR_DESC_SIZE(list[i]);
        state = VAL_HOST_RMI_ADDR_DESC_STATE(list[i]);
        count = VAL_HOST_RMI_ADDR_DESC_COUNT(list[i]);
        base = list[i] & VAL_HOST_RMI_ADDR_DESC_ADDR_MASK;

        if ((state != RMI_OP_MEM_DELEGATED) &&
            (state != RMI_OP_MEM_UNDELEGATED))
            return VAL_ERROR;

        block_shift = VAL_HOST_SRO_ADDR_SHIFT +
                      (size * (VAL_HOST_SRO_ADDR_SHIFT - 3U));
        if (block_shift >= 64U)
            return VAL_ERROR;

        block_size = 1UL << block_shift;

        if ((count == 0U) || ((base & (block_size - 1U)) != 0U) ||
            (count > (((uint64_t)-1) / block_size)))
            return VAL_ERROR;

        range_size = count * block_size;
        top = base + range_size;
        if (top < base)
            return VAL_ERROR;

        if ((state == RMI_OP_MEM_DELEGATED) &&
            (val_host_rmi_granule_range_undelegate(base, top) != RMI_SUCCESS))
        {
            LOG(ERROR, "Failed to undelegate reclaimed SRO memory\n");
            return VAL_ERROR;
        }

        val_host_mem_free((void *)base);
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    Reclaims memory returned by an in-progress SRO
 *   @param    handle    - SRO operation handle
 *   @param    args      - RMI output registers for the in-progress SRO
 *   @return   SUCCESS/FAILURE
**/
static uint32_t val_host_sro_reclaim(uint64_t handle, val_smc_param_ts *args,
                                     val_host_sro_alloc_fn alloc)
{
    uint64_t list_addr;
    uint64_t list_count = VAL_HOST_SRO_LIST_MAX_ENTRIES;
    uint64_t status;
    uint32_t i;
    uint32_t ret = VAL_ERROR;

    list_addr = (uint64_t)alloc(PAGE_SIZE, PAGE_SIZE);
    if (list_addr == 0)
    {
        LOG(ERROR, "Failed to allocate SRO reclaim list\n");
        return VAL_ERROR;
    }

    val_memset((void *)list_addr, 0, PAGE_SIZE);

    for (i = 0; i < VAL_HOST_SRO_MAX_ITERATIONS; i++)
    {
        *args = val_host_rmi_op_mem_reclaim_smc(handle, list_addr, list_count);
        status = RMI_STATUS(args->x0);

        if (status == RMI_BUSY)
            continue;

        if (status == RMI_INCOMPLETE)
        {
            if ((args->x1 != 0U) &&
                val_host_sro_release_reclaimed(list_addr, args->x1))
            {
                LOG(ERROR, "Failed to release reclaimed SRO memory\n");
                break;
            }

            val_memset((void *)list_addr, 0, PAGE_SIZE);
            ret = VAL_SUCCESS;
            break;
        }

        if (status == RMI_SUCCESS)
        {
            ret = VAL_SUCCESS;
            break;
        }

        *args = val_host_rmi_op_continue_smc(handle, RMI_CONTINUE_KEEP_GOING);
        ret = VAL_SUCCESS;
        break;
    }

    val_host_mem_free((void *)list_addr);

    if (ret != VAL_SUCCESS)
        LOG(ERROR, "RMI_OP_MEM_RECLAIM exceeded max iterations\n");

    return ret;
}

/**
 *   @brief    Completes a stateful RMI operation
 *   @param    args    - RMI output registers from the initiating ABI
 *   @return   SUCCESS/FAILURE
**/
static uint32_t val_host_sro_complete_handler(val_smc_param_ts *args, uint64_t handle,
                                              uint64_t region_base, uint64_t region_size,
                                              val_host_sro_alloc_fn alloc,
                                              bool add_to_mem_track)
{
    uint64_t mem_req;
    uint64_t status;
    uint32_t i;

    if (args == NULL)
        return VAL_ERROR;

    status = RMI_STATUS(args->x0);
    if (status != RMI_INCOMPLETE)
        return VAL_SUCCESS;

    for (i = 0; i < VAL_HOST_SRO_MAX_ITERATIONS; i++)
    {
        status = RMI_STATUS(args->x0);
        switch (status)
        {
            case RMI_BUSY:
                *args = val_host_rmi_op_continue_smc(handle, RMI_CONTINUE_KEEP_GOING);
                break;

            case RMI_SUCCESS:
                return VAL_SUCCESS;

            case RMI_INCOMPLETE:
                mem_req = VAL_HOST_SRO_MEM_REQ(args->x0);
                switch (mem_req)
                {
                    case RMI_OP_MEM_REQ_NONE:
                        *args = val_host_rmi_op_continue_smc(handle, RMI_CONTINUE_KEEP_GOING);
                        break;

                    case RMI_OP_MEM_REQ_DONATE:
                        if (val_host_sro_donate(handle, args, region_base, region_size,
                                                alloc, add_to_mem_track))
                            return VAL_ERROR;
                        break;

                    case RMI_OP_MEM_REQ_RECLAIM:
                        if (val_host_sro_reclaim(handle, args, alloc))
                            return VAL_ERROR;
                        break;

                    default:
                        LOG(ERROR, "Unsupported SRO memory request, mem_req=0x%x\n", mem_req);
                        return VAL_ERROR;
                }
                break;

            default:
                return VAL_SUCCESS;
        }
    }

    LOG(ERROR, "SRO completion exceeded max iterations\n");
    return VAL_ERROR;
}

/**
 *   @brief    Completes an SRO and returns final ABI status
 *   @param    args    - RMI output registers from the initiating ABI
 *   @return   Final ABI status in args->x0, or VAL_ERROR on helper failure
 **/
uint64_t val_host_sro_complete(val_smc_param_ts *args)
{
    if (args == NULL)
        return VAL_ERROR;

    return val_host_sro_complete_with_handle(args, args->x1);
}

/**
 *   @brief    Completes an SRO when its latest result does not return the handle
 *   @param    args      - Latest RMI output registers for the SRO
 *   @param    handle    - Handle returned by the initiating ABI
 *   @return   Final ABI status in args->x0, or VAL_ERROR on helper failure
 **/
uint64_t val_host_sro_complete_with_handle(val_smc_param_ts *args, uint64_t handle)
{
    if (args == NULL)
        return VAL_ERROR;

    if (val_host_sro_complete_handler(args, handle, 0, 0,
                                      val_host_mem_alloc, true) != VAL_SUCCESS)
    {
        if (args->x0 != 0)
            return args->x0;

        return VAL_ERROR;
    }

    return args->x0;
}

uint64_t val_host_sro_complete_tracking(val_smc_param_ts *args,
                                        uint64_t region_base, uint64_t region_size)
{
    if ((args == NULL) || (region_size == 0))
        return VAL_ERROR;

    if (val_host_sro_complete_handler(args, args->x1, region_base, region_size,
                                      val_host_tracking_alloc, false) != VAL_SUCCESS)
        return (args->x0 != 0) ? args->x0 : VAL_ERROR;

    return args->x0;
}
