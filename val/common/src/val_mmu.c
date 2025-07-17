/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_mmu.h"

#define min(a, b) (a < b)?a:b

#define PGT_DEBUG 1

uint64_t mmu_cfg[MMU_CFG_PARAM_MAX];

extern uint64_t security_state;
extern uint64_t realm_ipa_width;

/**
 *   @brief    Get supported PA range
 *   @param    void
 *   @return   PA Range
**/
uint64_t val_get_pa_range_supported(void)
{
    uint64_t pa = val_id_aa64mmfr0_el1_read();
    uint64_t pa_range = 0;

    pa = pa & 0x000F;

    switch (pa)
    {
    case 0:
        pa_range = 32;
        break;
    case 1:
        pa_range = 36;
        break;
    case 2:
        pa_range = 40;
        break;
    case 3:
        pa_range = 42;
        break;
    case 4:
        pa_range = 44;
        break;
    case 5:
        pa_range = 48;
        break;
    case 6:
        pa_range = 52;
        break;
    default:
        LOG(ERROR, "invalid ipa_width\n");
        break;
    }
    return pa_range;
}

/**
 * @brief Setup page table for image regions and device regions
 * @param ctx XLAT context.
 * @return status
**/

void val_setup_mmu(xlat_ctx_t *ctx)
{
    /* Write Page tables */
    init_xlat_tables_ctx(ctx);
}

/**
 * @brief Enable MMU through configuring MMU registers
 * @param ctx XLAT context,
 * @return status
**/

void val_enable_mmu(xlat_ctx_t *ctx)
{
    uint8_t  currentEL;

    currentEL = (val_read_current_el() & 0xc) >> 2;

    setup_mmu_cfg((uint64_t *)&mmu_cfg, 0, ctx->base_table,
                      ctx->pa_max_address, ctx->va_max_address, ctx->xlat_regime);


    mmu_cfg[MMU_CFG_TCR] |= (1ull << 39) | (1ull << 40);

    val_ttbr0_write(mmu_cfg[MMU_CFG_TTBR0], currentEL);
    val_tcr_write(mmu_cfg[MMU_CFG_TCR], currentEL);
    val_mair_write(mmu_cfg[MMU_CFG_MAIR], currentEL);

#ifdef PGT_DEBUG
    LOG(DBG, "val_setup_mmu: tcr=0x%lx\n", mmu_cfg[MMU_CFG_TCR]);
    LOG(DBG, "val_setup_mmu: ttbr=0x%lx\n", mmu_cfg[MMU_CFG_TTBR0]);
    LOG(DBG, "val_setup_mmu: mair=0x%lx\n", mmu_cfg[MMU_CFG_MAIR]);
#endif

/* Enable MMU */
    val_sctlr_write((1 << 0) |  // M=1 Enable the stage 1 MMU
                    (1 << 2) |  // C=1 Enable data and unified caches
                    (1 << 12) | // I=1 Enable instruction caches
                    val_sctlr_read(currentEL),
                    currentEL);
#ifdef PGT_DEBUG
    LOG(DBG, "val_enable_mmu: successful\n");
#endif

}

/**
 * @brief Creates a dynaminc page table mapping using the XLAT library
 * @param ctx XLAT context.
 * @param mem_desc Memory descriptor.
 * @return status
**/

int val_xlat_pgt_create(xlat_ctx_t *ctx, val_memory_region_descriptor_ts *mem_desc)
{
    mmap_region_t dynamic_region = MAP_REGION(mem_desc->physical_address, mem_desc->virtual_address,
                                                 mem_desc->length, mem_desc->attributes);

    return mmap_add_dynamic_region_ctx(ctx, &dynamic_region);
}

/**
 * @brief Converts raw index value into PIIndex[0:3] according to VMSAv8-64 format.
 * @param pi_index 4bit permision indirection index.
 * @return Page table descriptor with PIIndex[0:3] fields set.
**/
uint64_t val_pi_index_to_desc(uint64_t pi_index)
{
    return  INPLACE(ATTR_PI_INDEX_0, pi_index & 1) |
        INPLACE(ATTR_PI_INDEX_1, (pi_index >> 1) & 1) |
        INPLACE(ATTR_PI_INDEX_2, (pi_index >> 2) & 1) |
        INPLACE(ATTR_PI_INDEX_3, (pi_index >> 3) & 1);
}
