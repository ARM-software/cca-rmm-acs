/*
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "val_def.h"

OUTPUT_FORMAT(elf64-littleaarch64)
OUTPUT_ARCH(aarch64)
ENTRY(acs_realm_entry)

MEMORY {
    RAM (rwx): ORIGIN = 0x0, LENGTH = PLATFORM_REALM_IMAGE_SIZE
}

SECTIONS
{
    __ACS_IMAGE_BASE__ = .;

    ASSERT(. == ALIGN(PAGE_SIZE),
           "TEXT_START address is not aligned to PAGE_SIZE.")
    .text : {
        __TEXT_START__ = .;
        *val_realm_entry.S.o(.text*)
        *(.text*)
        . = NEXT(PAGE_SIZE);
        __TEXT_END__ = .;
    }

    .rodata : {
        . = ALIGN(PAGE_SIZE);
        __RODATA_START__ = .;
        *(.rodata*)

        . = NEXT(PAGE_SIZE);
        __RODATA_END__ = .;

    }

    .data : {
        . = ALIGN(PAGE_SIZE);
        __DATA_START__ = .;
        *(.data*)

        . = ALIGN(8);
        __ACS_GOT_START__ = .;
        *(.got)
        __ACS_GOT_END__ = .;

        . = NEXT(PAGE_SIZE);
        __DATA_END__ = .;
    }

    /*
     * .rela.dyn needs to come after .data for the read-elf utility to
     * parse this section correctly.
     */
    .rela.dyn ALIGN(8) : {
        __ACS_RELA_START__ = .;
        *(.rela*)
        __ACS_RELA_END__ = .;
    } >RAM

    .bss (NOLOAD) : {
        . = ALIGN(PAGE_SIZE);
        __BSS_START__ = .;
        *(SORT_BY_ALIGNMENT(.bss*))
        *(COMMON)
        . = NEXT(PAGE_SIZE);
    } >RAM

    pgt_entries ALIGN(PAGE_SIZE) : {
            *(xlat_static_tables)
    } >RAM
    __BSS_END__ = .;

    __ACS_IMAGE_END__ = .;
}
