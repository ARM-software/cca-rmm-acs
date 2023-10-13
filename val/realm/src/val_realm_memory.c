/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "val_realm_memory.h"
#include "val_realm_rsi.h"

REGISTER_XLAT_CONTEXT2(acs_realm,
		       REALM_MEM_REGIONS,
		       ACS_REALM_CTX_MAX_XLAT_TABLES,
		       REALM_MAX_VIRT_ADDR_SPACE_SIZE, REALM_MAX_PHY_ADDR_SPACE_SIZE,
		       EL1_EL0_REGIME, ACS_REALM_IMAGE_XLAT_SECTION_NAME,
		       ACS_REALM_IMAGE_BASE_XLAT_SECTION_NAME);

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

#define REALM_TEXT MAP_REGION_FLAT(                              \
                                TEXT_START,                     \
                                (TEXT_END - TEXT_START),        \
                                MT_CODE | MT_REALM)
#define REALM_RO MAP_REGION_FLAT(                                \
                                RODATA_START,                   \
                                (RODATA_END - RODATA_START),    \
                                MT_RO_DATA | MT_REALM)
#define REALM_RW MAP_REGION_FLAT(                                \
                                DATA_START,                     \
                                (DATA_END - DATA_START),        \
                                MT_RW_DATA | MT_REALM)
#define REALM_BSS MAP_REGION_FLAT(                               \
                                BSS_START,                      \
                                (BSS_END - BSS_START),          \
                                MT_RW_DATA | MT_REALM)


/**
 *   @brief    Add regions assigned to realm into its translation table data structure.
 *   @param    void
 *   @return   void
**/

void val_realm_add_mmap(void)
{
    uint64_t ipa_width = val_realm_get_ipa_width();
    mmap_region_t realm_region[REALM_MEM_REGIONS] = {
        REALM_TEXT,
        REALM_RO,
        REALM_RW,
        REALM_BSS
    };

    mmap_add_ctx(&acs_realm_xlat_ctx, realm_region);

    mmap_region_t plat_shared_region =
                        MAP_REGION_FLAT((uint64_t)val_get_shared_region_base_ipa(ipa_width),
                        PLATFORM_SHARED_REGION_SIZE,
                        MT_RW_DATA | MT_NS);

    mmap_add_region_ctx(&acs_realm_xlat_ctx, &plat_shared_region);

}

/**
 *   @brief    Return realm XLAT context.
 *   @param    void
 *   @return   XLAT context.
**/
xlat_ctx_t *val_realm_get_xlat_ctx(void)
{
    return &acs_realm_xlat_ctx;
}

/**
 *   @brief    Wrapper function to create dynamic Realm Page tables.
 *   @param    mem_desc Memory descriptor
 *   @return   status.
**/
int val_realm_pgt_create(val_memory_region_descriptor_ts *mem_desc)
{

    return val_xlat_pgt_create(&acs_realm_xlat_ctx, mem_desc);
}

/**
 *   @brief    Updates Realm XLAT contexts with new maximum VA and PA size.
 *   @param    ias Input Address size
 *   @param    oas Output Address size
 *   @return   none
**/
void val_realm_update_xlat_ctx_ias_oas(uint64_t ias, uint64_t oas)
{
    acs_realm_xlat_ctx.va_max_address = ias;
    acs_realm_xlat_ctx.pa_max_address = oas;
    acs_realm_xlat_ctx.base_level = GET_XLAT_TABLE_LEVEL_BASE(ias);
    return;
}

/**
 *   @brief    Reads Page descriptor attributes from Realm Page tables.
 *   @param    va Virtual address of the page to read.
 *   @param    attr Pointer to store attributes
 *   @return   none
**/
void val_realm_read_attributes(uint64_t va, uint32_t *attr)
{
    xlat_get_mem_attributes_ctx(&acs_realm_xlat_ctx, va, attr);
}
