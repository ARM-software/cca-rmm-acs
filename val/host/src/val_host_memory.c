/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "val_host_memory.h"

REGISTER_XLAT_CONTEXT2(acs_host,
		       HOST_MEM_REGIONS,
		       ACS_HOST_CTX_MAX_XLAT_TABLES,
		       PLAT_VIRT_ADDR_SPACE_SIZE, PLAT_PHY_ADDR_SPACE_SIZE,
		       EL2_REGIME, ACS_HOST_IMAGE_XLAT_SECTION_NAME,
		       ACS_HOST_IMAGE_BASE_XLAT_SECTION_NAME);

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

#define HOST_TEXT MAP_REGION_FLAT(                              \
                                TEXT_START,                     \
                                (TEXT_END - TEXT_START),        \
                                MT_CODE | MT_NS)
#define HOST_RO MAP_REGION_FLAT(                                \
                                RODATA_START,                   \
                                (RODATA_END - RODATA_START),    \
                                MT_RO_DATA | MT_NS)
#define HOST_RW MAP_REGION_FLAT(                                \
                                DATA_START,                     \
                                (DATA_END - DATA_START),        \
                                MT_RW_DATA | MT_NS)
#define HOST_BSS MAP_REGION_FLAT(                               \
                                BSS_START,                      \
                                (BSS_END - BSS_START),          \
                                MT_RW_DATA | MT_NS)
#define MEMORY_POOL MAP_REGION2(                            \
                                PLATFORM_MEMORY_POOL_BASE,      \
                                PLATFORM_MEMORY_POOL_BASE,      \
                                PLATFORM_MEMORY_POOL_SIZE,      \
                                MT_RW_DATA | MT_NS,              \
                                0x1000)
#define NS_UART MAP_REGION_FLAT(                                \
                                PLATFORM_NS_UART_BASE,          \
                                PLATFORM_NS_UART_SIZE,          \
                                MT_DEVICE_RW | MT_NS)
#define NVM     MAP_REGION_FLAT(                                \
                                PLATFORM_NVM_BASE,              \
                                PLATFORM_NVM_SIZE,              \
                                MT_DEVICE_RW | MT_NS)
#define WDOG    MAP_REGION_FLAT(                                \
                                PLATFORM_WDOG_BASE,             \
                                PLATFORM_WDOG_SIZE,             \
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
#define NS_WDOG MAP_REGION_FLAT(                                \
                                PLATFORM_NS_WD_BASE,            \
                                PLATFORM_NS_WD_SIZE,            \
                                MT_DEVICE_RW | MT_NS)

/**
 *   @brief    Add regions assigned to host into its translation table data structure.
 *   @param    void
 *   @return   void
**/

void val_host_add_mmap(void)
{
    mmap_region_t host_regions[HOST_MEM_REGIONS] = {
            NS_UART,
            WDOG,
            NS_WDOG,
            GICC,
            GICD,
            GICR,
            NVM,
            HOST_TEXT,
            HOST_RO,
            HOST_RW,
            HOST_BSS,
            MEMORY_POOL
    };

    mmap_add_ctx(&acs_host_xlat_ctx, host_regions);
}

/**
 *   @brief    Return host XLAT context.
 *   @param    void
 *   @return   XLAT context.
**/
xlat_ctx_t *val_host_get_xlat_ctx(void)
{
    return &acs_host_xlat_ctx;
}

/**
 *   @brief    Wrapper function to create dynamic Host Page tables.
 *   @param    mem_desc Memory descriptor
 *   @return   status.
**/
int val_host_pgt_create(val_memory_region_descriptor_ts *mem_desc)
{

    return val_xlat_pgt_create(&acs_host_xlat_ctx, mem_desc);
}

/**
 *   @brief    Reads Page descriptor attributes from Host Page tables.
 *   @param    va Virtual address of the page to read.
 *   @param    attr Pointer to store attributes
 *   @return   none
**/
void val_host_read_atributes(uint64_t va, uint32_t *attr)
{
    xlat_get_mem_attributes_ctx(&acs_host_xlat_ctx, va, attr);
}

/**
 *   @brief    Updates Page descriptor attributes from Host Page tables.
 *   @param    size Number of pages to update.
 *   @param    va Virtual address of the page to read.
 *   @param    attr Desired attributes
 *   @return   none
**/

int val_host_update_attributes(uint64_t size, uint64_t va, uint32_t attr)
{
    return xlat_change_mem_attributes_ctx(&acs_host_xlat_ctx, va, size, attr);
}
