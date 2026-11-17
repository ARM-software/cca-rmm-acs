/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_HOST_SRO_H_
#define _VAL_HOST_SRO_H_

#include "val.h"
#include "val_smc.h"

#define VAL_HOST_SRO_MAX_ITERATIONS 1024

#if (PAGE_SIZE == PAGE_SIZE_4K)
#define VAL_HOST_SRO_ADDR_SHIFT PAGE_BITS_4K
#elif (PAGE_SIZE == PAGE_SIZE_16K)
#define VAL_HOST_SRO_ADDR_SHIFT PAGE_BITS_16K
#elif (PAGE_SIZE == PAGE_SIZE_64K)
#define VAL_HOST_SRO_ADDR_SHIFT PAGE_BITS_64K
#else
#error "Unsupported PAGE_SIZE"
#endif

#define VAL_HOST_SRO_DESC_MAX_COUNT ((1UL << (VAL_HOST_SRO_ADDR_SHIFT - 2)) - 1UL)

/* RmiAddrRangeDesc fields. */
#define VAL_HOST_RMI_ADDR_DESC_SIZE(desc) \
    VAL_EXTRACT_BITS((desc), 0, 1)
#define VAL_HOST_RMI_ADDR_DESC_COUNT(desc) \
    VAL_EXTRACT_BITS((desc), 2, VAL_HOST_SRO_ADDR_SHIFT - 1U)
#define VAL_HOST_RMI_ADDR_DESC_STATE(desc) \
    VAL_EXTRACT_BITS((desc), 62, 63)
#define VAL_HOST_RMI_ADDR_DESC_ADDR_MASK \
    (((1UL << 52) - 1UL) & ~((1UL << VAL_HOST_SRO_ADDR_SHIFT) - 1UL))

static inline uint64_t val_host_rmi_addr_range_desc(uint64_t addr, uint64_t count,
                                                     uint64_t size, uint64_t state)
{
    return (addr & VAL_HOST_RMI_ADDR_DESC_ADDR_MASK) |
           ((count & VAL_HOST_SRO_DESC_MAX_COUNT) << 2) |
           (size & 0x3UL) | ((state & 0x3UL) << 62);
}

#define VAL_HOST_SRO_LIST_MAX_ENTRIES (PAGE_SIZE / sizeof(uint64_t))
#define VAL_HOST_SRO_MEM_REQ(result) VAL_EXTRACT_BITS((result), 8, 9)
#define VAL_HOST_SRO_DONATE_REQ_SIZE(req) VAL_EXTRACT_BITS((req), 0, 1)
#define VAL_HOST_SRO_DONATE_REQ_COUNT(req) VAL_EXTRACT_BITS((req), 2, 15)
#define VAL_HOST_SRO_DONATE_REQ_CONTIG(req) VAL_EXTRACT_BITS((req), 16, 16)
#define VAL_HOST_SRO_DONATE_REQ_STATE(req) VAL_EXTRACT_BITS((req), 17, 18)
#define VAL_HOST_SRO_MEM_FLAGS_SIZE(flags) VAL_EXTRACT_BITS((flags), 0, 1)
#define VAL_HOST_SRO_MEM_FLAGS_STATE(flags) VAL_EXTRACT_BITS((flags), 3, 4)

uint64_t val_host_sro_complete(val_smc_param_ts *args);
uint64_t val_host_sro_complete_with_handle(val_smc_param_ts *args, uint64_t handle);
uint64_t val_host_sro_complete_tracking(val_smc_param_ts *args,
                                        uint64_t region_base, uint64_t region_size);
uint32_t val_host_sro_release_reclaimed(uint64_t list_addr, uint64_t list_count);

#endif /* _VAL_HOST_SRO_H_ */
