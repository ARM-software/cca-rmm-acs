/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "val_host_memory.h"

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
 *   @brief    Add regions assigned to host into its translation table data structure.
 *   @param    void
 *   @return   void
**/
void val_host_add_mmap(void)
{
    /* Host Image region */
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

    /* Memory Pool region */
    val_mmap_add_region(PLATFORM_MEMORY_POOL_BASE,
                        PLATFORM_MEMORY_POOL_BASE,
                        PLATFORM_MEMORY_POOL_SIZE,
                        ATTR_RW_DATA | ATTR_NS);

    /* Device region */
    val_mmap_add_region(PLATFORM_NS_UART_BASE,
                        PLATFORM_NS_UART_BASE,
                        PLATFORM_NS_UART_SIZE,
                        ATTR_DEVICE_RW | ATTR_NS);
    val_mmap_add_region(PLATFORM_NVM_BASE,
                        PLATFORM_NVM_BASE,
                        PLATFORM_NVM_SIZE,
                        ATTR_DEVICE_RW | ATTR_NS);
    val_mmap_add_region(PLATFORM_WDOG_BASE,
                        PLATFORM_WDOG_BASE,
                        PLATFORM_WDOG_SIZE,
                        ATTR_DEVICE_RW | ATTR_NS);

    val_mmap_add_region(GICD_BASE, GICD_BASE, GICD_SIZE, ATTR_DEVICE_RW | ATTR_NS);
    val_mmap_add_region(GICR_BASE, GICR_BASE, GICD_SIZE, ATTR_DEVICE_RW | ATTR_NS);
    val_mmap_add_region(GICC_BASE, GICC_BASE, GICD_SIZE, ATTR_DEVICE_RW | ATTR_NS);

    val_mmap_add_region(PLATFORM_NS_WD_BASE,
                        PLATFORM_NS_WD_BASE,
                        PLATFORM_NS_WD_SIZE,
                        ATTR_DEVICE_RW | ATTR_NS);
}
