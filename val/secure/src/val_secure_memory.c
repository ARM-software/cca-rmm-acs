/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "val_secure_memory.h"

REGISTER_XLAT_CONTEXT2(acs_secure,
		       SECURE_MEM_REGIONS,
		       ACS_SECURE_CTX_MAX_XLAT_TABLES,
		       PLAT_VIRT_ADDR_SPACE_SIZE, PLAT_PHY_ADDR_SPACE_SIZE,
		       EL2_REGIME, ACS_SECURE_IMAGE_XLAT_SECTION_NAME,
		       ACS_SECURE_IMAGE_BASE_XLAT_SECTION_NAME);

/* Linker symbols used to figure out the memory layout of secure partition. */
extern uintptr_t __TEXT_START__, __TEXT_END__;
#define TEXT_START    ((uintptr_t)&__TEXT_START__)
#define TEXT_END      ((uintptr_t)&__TEXT_END__)

extern uintptr_t __RODATA_START__, __RODATA_END__;
#define RODATA_START  ((uintptr_t)&__RODATA_START__)
#define RODATA_END    ((uintptr_t)&__RODATA_END__)

extern uintptr_t __DATA_START__, __DATA_END__;
#define DATA_START    ((uintptr_t)&__DATA_START__)
#define DATA_END      ((uintptr_t)&__DATA_END__)

extern uintptr_t __BSS_START__, __BSS_END__;
#define BSS_START  ((uintptr_t)&__BSS_START__)
#define BSS_END    ((uintptr_t)&__BSS_END__)

#define SECURE_TEXT MAP_REGION_FLAT(                              \
                                TEXT_START,                     \
                                (TEXT_END - TEXT_START),        \
                                MT_CODE | MT_SECURE)
#define SECURE_RO MAP_REGION_FLAT(                                \
                                RODATA_START,                   \
                                (RODATA_END - RODATA_START),    \
                                MT_RO_DATA | MT_SECURE)
#define SECURE_RW MAP_REGION_FLAT(                                \
                                DATA_START,                     \
                                (DATA_END - DATA_START),        \
                                MT_RW_DATA | MT_SECURE)
#define SECURE_BSS MAP_REGION_FLAT(                               \
                                BSS_START,                      \
                                (BSS_END - BSS_START),          \
                                MT_RW_DATA | MT_SECURE)
#define MEMORY_POOL MAP_REGION_FLAT(                            \
                                PLATFORM_MEMORY_POOL_BASE,      \
                                PLATFORM_MEMORY_POOL_SIZE,      \
                                MT_RW_DATA | MT_NS)
#define NS_UART MAP_REGION_FLAT(                                \
                                PLATFORM_NS_UART_BASE,          \
                                PLATFORM_NS_UART_SIZE,          \
                                MT_DEVICE_RW | MT_NS)
#define GICD    MAP_REGION_FLAT(                               \
                                GICD_BASE,                      \
                                GICD_SIZE,                      \
                                MT_DEVICE_RW | MT_NS)
#define GICR    MAP_REGION_FLAT(                                \
                                GICR_BASE,                      \
                                GICR_SIZE,                      \
                                MT_DEVICE_RW | MT_NS)
#define GICC    MAP_REGION_FLAT(                                \
                                GICC_BASE,                      \
                                GICC_SIZE,                      \
                                MT_DEVICE_RW | MT_NS)
#define TWDOG    MAP_REGION_FLAT(                                \
                                PLATFORM_SP805_TWDOG_BASE,       \
                                PLATFORM_TWDOG_SIZE,              \
                                MT_DEVICE_RW | MT_NS)
/**
 *   @brief    Add regions assigned to secure into its translation table data structure.
 *   @param    void
 *   @return   void
**/
void val_secure_add_mmap(void)
{

    struct mmap_region secure_regions[SECURE_MEM_REGIONS] = {
            NS_UART,
            TWDOG,
            GICC,
            GICD,
            GICR,
            SECURE_TEXT,
            SECURE_RO,
            SECURE_RW,
            SECURE_BSS,
            MEMORY_POOL
    };

    mmap_add_ctx(&acs_secure_xlat_ctx, secure_regions);
}

/**
 *   @brief    Return secure XLAT context.
 *   @param    void
 *   @return   XLAT context.
**/
xlat_ctx_t *val_secure_get_xlat_ctx(void)
{
    return &acs_secure_xlat_ctx;
}

