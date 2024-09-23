/*
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
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

    /* Lower Limit for fixup */
    mov x0, xzr
    /* acs_realm_base and acs_realm_end are 4KB aligned hence adrp is enough */
    adrp x2, __ACS_IMAGE_BASE__
    adrp x1, __ACS_IMAGE_END__
    /* Upper Limit for fixup (acs_realm_end - acs_realm_base) */
    sub x1, x1, x2

    /*
     * Since ACS image will be compiled to start at 0x0, the current
         * PC relative `acs_realm_base` loaded in x2 will be the Diff(S)
     * to be applied to the fixups.
     */
    cbz x2, 4f /* Diff(S) = 0. No relocation needed */

    adrp x6, __ACS_GOT_START__
    add x6, x6, :lo12:__ACS_GOT_START__
    adrp x7, __ACS_GOT_END__
    add x7, x7, :lo12:__ACS_GOT_END__

    /*
     * GOT is an array of 64_bit addresses which must be fixed up as
     * new_addr = old_addr + Diff(S).
     * The new_addr is the address currently the binary is executing from
     * and old_addr is the address at compile time.
     */
1:  ldr x3, [x6]
    /* Skip adding offset if address is < lower limit */
    cmp x3, x0
    b.lo 2f

    /* Skip adding offset if address is > upper limit */
    cmp x3, x1
    b.hi 2f
    add x3, x3, x2
    str x3, [x6]

2:  add x6, x6, #8
    cmp x6, x7
    b.lo 1b

    /* Starting dynamic relocations */
3:  adrp x6, __ACS_RELA_START__
    add x6, x6, :lo12:__ACS_RELA_START__
    adrp x7, __ACS_RELA_END__
    add x7, x7, :lo12:__ACS_RELA_END__

    /*
     * According to ELF-64 specification, the RELA data structure is as
     * follows:
     *  typedef struct {
     *      Elf64_Addr r_offset;
     *      Elf64_Xword r_info;
     *      Elf64_Sxword r_addend;
     *  } Elf64_Rela;
     *
     * r_offset is address of reference
     * r_info is symbol index and type of relocation (in this case
     * code 1027 which corresponds to R_AARCH64_RELATIVE).
     * r_addend is constant part of expression.
     *
     * Size of Elf64_Rela structure is 24 bytes.
     */

1:  ldr x3, [x6, #8]    /* r_info */
    /* Skip R_AARCH64_NONE entry with code 0 */
    cbz x3, 2f

    ldr x4, [x6, #16]   /* r_addend */

    /* Skip adding offset if r_addend is < lower limit */
    cmp x4, x0
    b.lo    2f

    /* Skip adding offset if r_addend entry is > upper limit */
    cmp x4, x1
    b.hi    2f

    ldr x3, [x6]    /* r_offset */
    add x4, x4, x2  /* Diff(S) + r_addend */
    str x4, [x3, x2]

2:  add x6, x6, #24
    cmp x6, x7
    b.lo    1b

4:
    ret
