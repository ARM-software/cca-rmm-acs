/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_def.h"

    .extern val_main
    .extern vector_table
    .extern val_fixup_symbol_table
    .extern val_inv_dcache_range
    .extern val_primary_mpidr

    .cfi_sections .debug_frame
    .globl    acs_secure_entry
    .section .text.acs_secure_entry, "ax"
acs_secure_entry:

   /* Secure code is designed such way that it can run at EL2S or EL1S */

   /* Install vector table */
    adr     x0, vector_table
#if (PLATFORM_SECURE_IMAGE_EL == 0x2)
    msr     vbar_el2, x0
#else
    msr     vbar_el1, x0
#endif
    isb

   /* Set x19 = 1 for primary cpu
    * Set x19 = 0 for secondary cpu
    */
    adr x18, val_primary_mpidr
    ldr x0, [x18]
    mov   x2, #INVALID_MPIDR
    cmp   x2, x0
    b.ne  secondary_cpu_entry

    mov   x19, #1

#ifdef PLATFORM_BOOT_CACHE_INVALIDATE
    /*
     * Invalidate the instr cache for the code regions.
     * This prevents re-use of stale data cache entries from
     * prior bootloader stages.
     */
    adr x0, __TEXT_START__
    adr x1, __TEXT_END__
    sub x1, x1, x0
    bl  val_inv_icache_range

    /*
     * Invalidate the data cache for the data regions.
     * This prevents re-use of stale data cache entries from
     * prior bootloader stages.
     */
    adr x0, __RODATA_START__
    adr x1, __BSS_END__
    sub x1, x1, x0
    bl  val_inv_dcache_range
#endif

    /* Enable I-Cache */
#if (PLATFORM_SECURE_IMAGE_EL == 0x2)
    mrs     x0, sctlr_el2
    orr     x0, x0, #SCTLR_I_BIT
    msr     sctlr_el2, x0
#else
    mrs     x0, sctlr_el1
    orr     x0, x0, #SCTLR_I_BIT
    msr     sctlr_el1, x0
#endif
    isb

    /* Save the primary cpu mpidr */
    adr x18, val_primary_mpidr
    mrs x0, mpidr_el1
    str x0, [x18]

   /* Clear BSS */
   adr x0, __BSS_START__
   adr x1, __BSS_END__
   sub x1, x1, x0
1:
   stp xzr, xzr, [x0]
   add x0, x0, #16
   sub x1, x1, #16
   cmp xzr, x1
   b.ne 1b

   b  0f

secondary_cpu_entry:
    mov   x19, #0

   /* Enable I-Cache */
#if (PLATFORM_SECURE_IMAGE_EL == 0x2)
    mrs     x0, sctlr_el2
    orr     x0, x0, #SCTLR_I_BIT
    msr     sctlr_el2, x0
#else
    mrs     x0, sctlr_el1
    orr     x0, x0, #SCTLR_I_BIT
    msr     sctlr_el1, x0
#endif
    isb

0:
    /* Setup the dummy stack to call val_get_cpuid C fn */
    adr   x1, dummy_stack_end
    mov   sp, x1

    mrs   x0, mpidr_el1
    bl    val_get_cpuid

    /* Now setup the stack pointer with actual stack addr
       for the logic cpuid return by val_get_cpuid */
    adr    x1, stacks_end
    mov    x2, #STACK_SIZE
    mul    x2, x0, x2
    sub    sp, x1, x2

    /* And jump to the C entrypoint. */
    mov  x0, x19
    b      val_secure_main

.section .bss.stacks
  .global stacks_start
  .global stacks_end
    .balign CACHE_WRITEBACK_GRANULE
stacks_start:
    .fill    STACK_SIZE * PLATFORM_CPU_COUNT
stacks_end:
    .fill    STACK_SIZE
dummy_stack_end:
