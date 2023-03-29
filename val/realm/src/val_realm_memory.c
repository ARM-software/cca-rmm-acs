/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "val_realm_memory.h"
#include "val_realm_rsi.h"

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

/**
 *   @brief    Add regions assigned to realm into its translation table data structure.
 *   @param    void
 *   @return   void
**/
void val_realm_add_mmap(void)
{
    uint64_t ipa_width = val_realm_get_ipa_width();

    /* Realm Image region */
    val_mmap_add_region(TEXT_START, TEXT_START,
                        (TEXT_END - TEXT_START),
                        ATTR_CODE | ATTR_NS);
    val_mmap_add_region(RODATA_START, RODATA_START,
                        (RODATA_END - RODATA_START),
                        ATTR_RO_DATA | ATTR_NS);
    val_mmap_add_region(DATA_START, DATA_START,
                        (DATA_END - DATA_START),
                        ATTR_RW_DATA | ATTR_NS);
    val_mmap_add_region(BSS_START, BSS_START,
                        (BSS_END - BSS_START),
                        ATTR_RW_DATA | ATTR_NS);

    /* NS Shared region */
    val_mmap_add_region((uint64_t)val_get_shared_region_base_ipa(ipa_width),
                        (uint64_t)val_get_shared_region_base_ipa(ipa_width),
                        PLATFORM_SHARED_REGION_SIZE,
                        ATTR_RW_DATA | ATTR_NS);

}

