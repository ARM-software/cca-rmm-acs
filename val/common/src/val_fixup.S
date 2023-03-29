/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_def.h"

  .section .text.fixup, "ax"

/* ---------------------------------------------------------------------------
 * Helper to fixup Global Offset table (GOT) at runtime.
 *
 * This function is used as the partition is compiled with -fpie
 * and linked with -pie options. We rely on the linker script exporting
 * appropriate markers for start and end of the section. For GOT, we
 * expect __GOT_START__ and __GOT_END__.
 *
 * The function takes the limits of the memory to apply fixups to as
 * arguments (which is usually the limits of the relocable BL image).
 *   x0 -  the start of the fixup region
 *   x1 -  the limit of the fixup region
 * These addresses have to be max page aligned(64k).
 * ---------------------------------------------------------------------------
 */

    .globl    val_fixup_symbol_table
val_fixup_symbol_table:

    mov    x6, x0
    mov    x7, x1

    /* Test if the limits are page aligned */
    orr    x0, x0, x1
    tst    x0, #(PAGE_ALIGNMENT - 1)
    b.eq   1f
    b .
1:

    /*
     * Calculate the offset based on return address in x30.
     * Assume that this function is called within a page at the start of
     * fixup region.
     */
    and    x2, x30, #~(PAGE_ALIGNMENT - 1)
    sub    x0, x2, x6    /* Diff(S) = Current Address - Compiled Address */
    adrp   x1, __GOT_START__
    add    x1, x1, :lo12:__GOT_START__
    adrp   x2, __GOT_END__
    add    x2, x2, :lo12:__GOT_END__

    /*
     * GOT is an array of 64_bit addresses which must be fixed up as
     * new_addr = old_addr + Diff(S).
     * The new_addr is the address currently the binary is executing from
     * and old_addr is the address at compile time.
     */

2:
    ldr    x3, [x1]
    /* Skip adding offset if address is < lower limit */
    cmp    x3, x6
    b.lo   3f
    /* Skip adding offset if address is >= upper limit */
    cmp    x3, x7
    b.ge   3f
    add    x3, x3, x0
    str    x3, [x1]
3:
    add    x1, x1, #8
    cmp    x1, x2
    b.lo   2b

    ret
