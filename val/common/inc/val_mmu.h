/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_MEMORY_H_
#define _VAL_MEMORY_H_

#include "val.h"
#include "val_sysreg.h"
#include "val_framework.h"
#include "xlat_tables_v2.h"

#define MAX_REGION_COUNT 15
#define ATTR_NORMAL_NONCACHEABLE (0x0ull << 2)
#define ATTR_NORMAL_WB_WA_RA      (0x1ull << 2)
#define ATTR_DEVICE               (0x2ull << 2)
#define ATTR_NORMAL_WB            (0x1ull << 3)

/* Stage 1 Inner and Outer Cacheability attribute encoding without TEX remap */
#define ATTR_S1_NONCACHEABLE  (0x0ull << 2)
#define ATTR_S1_WB_WA_RA       (0x1ull << 2)
#define ATTR_S1_WT_RA          (0x2ull << 2)
#define ATTR_S1_WB_RA          (0x3ull << 2)

/* Stage 2 MemAttr[1:0] encoding for Normal memory */
#define ATTR_S2_INNER_NONCACHEABLE   (0x1ull << 2)
#define ATTR_S2_INNER_WT_CACHEABLE   (0x2ull << 2)
#define ATTR_S2_INNER_WB_CACHEABLE   (0x3ull << 2)

#define ATTR_NS   (0x1ull << 5)
#define ATTR_S    (0x0ull << 5)

#define ATTR_STAGE1_AP_RW (0x1ull << 6)
#define ATTR_STAGE2_AP_RW (0x3ull << 6)
#define ATTR_STAGE2_MASK  (0x3ull << 6 | 0x1ull << 4)
#define ATTR_STAGE2_MASK_RO  (0x1ull << 6 | 0x1ull << 4)
#define ATTR_STAGE2_MASK_WO  (0x2ull << 6 | 0x1ull << 4)

#define ATTR_NON_SHARED     (0x0ull << 8)
#define ATTR_OUTER_SHARED   (0x2ull << 8)
#define ATTR_INNER_SHARED   (0x3ull << 8)

#define ATTR_AF   (0x1ull << 10)
#define ATTR_nG   (0x1ull << 11)

#define ATTR_DBM   (0x1ull << 51)

#define ATTR_UXN    (0x1ull << 54)
#define ATTR_PXN    (0x1ull << 53)

#define ATTR_PRIV_RW        (0x0ull << 6)
#define ATTR_PRIV_RO        (0x2ull << 6)
#define ATTR_USER_RW        (0x1ull << 6)
#define ATTR_USER_RO        (0x3ull << 6)

#define ATTR_CODE           (ATTR_S1_WB_WA_RA | ATTR_USER_RO | \
                              ATTR_AF | ATTR_INNER_SHARED)
#define ATTR_RO_DATA        (ATTR_S1_WB_WA_RA | ATTR_USER_RO | \
                              ATTR_UXN | ATTR_PXN | ATTR_AF | \
                              ATTR_INNER_SHARED)
#define ATTR_RW_DATA        (ATTR_S1_WB_WA_RA | \
                              ATTR_USER_RW | ATTR_UXN | ATTR_PXN | ATTR_AF \
                              | ATTR_INNER_SHARED)

#define ATTR_RO_DATA_AF      (ATTR_S1_WB_WA_RA | ATTR_USER_RO | \
                              ATTR_UXN | ATTR_PXN | ATTR_INNER_SHARED)

#define ATTR_DEVICE_RW      (ATTR_DEVICE | ATTR_USER_RW | ATTR_UXN | \
                              ATTR_PXN | ATTR_AF | ATTR_INNER_SHARED)

#define ATTR_RW_DATA_NC      (ATTR_S1_NONCACHEABLE | \
                              ATTR_USER_RW | ATTR_UXN | ATTR_PXN | ATTR_AF \
                              | ATTR_INNER_SHARED)

#define PGT_STAGE1 1
#define PGT_STAGE2 2

#define PGT_IPS     0x2ull
#define PGT_T0SZ    (64 - PGT_IAS)

#define PGT_ENTRY_TABLE_MASK (0x1 << 1)
#define PGT_ENTRY_VALID_MASK  0x1
#define PGT_ENTRY_PAGE_MASK  (0x1 << 1)
#define PGT_ENTRY_BLOCK_MASK (0x0 << 1)

#define IS_PGT_ENTRY_PAGE(val) (val & 0x2)
#define IS_PGT_ENTRY_BLOCK(val) !(val & 0x2)

#define PGT_DESC_SIZE 8
#define PGT_DESC_ATTR_UPPER_MASK (((0x1ull << 12) - 1) << 52)
#define PGT_DESC_ATTR_LOWER_MASK (((0x1ull << 10) - 1) << 2)
#define PGT_DESC_ATTRIBUTES_MASK \
    (PGT_DESC_ATTR_UPPER_MASK | PGT_DESC_ATTR_LOWER_MASK)
#define PGT_DESC_ATTRIBUTES(val) (val & PGT_DESC_ATTRIBUTES_MASK)
#define OA(addr) (addr & (((0x1ull << 36) - 1) << PAGE_BITS_4K))

typedef struct val_mem_info {
    uint32_t index;
    uint64_t base_address;
    uint64_t total_size;
    uint32_t hole;
} val_mem_info_ts;

typedef struct val_mem_alloc_info {
    uint64_t address;
    uint64_t size;
} val_mem_alloc_info_ts;

typedef struct {
    uint64_t virtual_address;
    uint64_t physical_address;
    uint64_t length;
    uint64_t attributes;
} val_memory_region_descriptor_ts;

typedef struct {
    uint32_t ias;
    uint32_t oas;
    uint32_t stage;
    uint64_t *ttbr;
} val_pgt_descriptor_ts;

typedef struct {
    uint64_t *tt_base;
    uint64_t input_base;
    uint64_t input_top;
    uint64_t output_base;
    uint32_t level;
    uint32_t size_log2;
    uint32_t nbits;
} val_tt_descriptor_ts;

void val_setup_mmu(xlat_ctx_t *ctx);
void val_enable_mmu(xlat_ctx_t *ctx);
uint64_t val_get_pa_range_supported(void);
int val_xlat_pgt_create(xlat_ctx_t *ctx, val_memory_region_descriptor_ts *mem_desc);
#endif /* _VAL_MEMORY_H_ */
