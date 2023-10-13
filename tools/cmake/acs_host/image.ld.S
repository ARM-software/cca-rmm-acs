/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "val_def.h"

IMAGE_BASE = PLATFORM_HOST_IMAGE_BASE;
OUTPUT_FORMAT(elf64-littleaarch64)
OUTPUT_ARCH(aarch64)
ENTRY(acs_host_entry)

MEMORY {
    RAM (rwx): ORIGIN = IMAGE_BASE, LENGTH = PLATFORM_HOST_IMAGE_SIZE
}

SECTIONS
{
    . = IMAGE_BASE;

    ASSERT(. == ALIGN(PAGE_SIZE),
           "TEXT_START address is not aligned to PAGE_SIZE.")
    .text : {
        __TEXT_START__ = .;
        *val_host_entry.S.o(.text*)
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
        __GOT_START__ = .;
        *(.got)
        __GOT_END__ = .;

        . = NEXT(PAGE_SIZE);
        __DATA_END__ = .;
    }

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
}
