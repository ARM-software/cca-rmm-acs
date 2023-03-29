
/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/**
 * This is a generic handler for exceptions taken at the current EL while using
 * SPx. It saves volatile registers, calls the C handler, restores volatile
 * registers, then returns.
 *
 * Saving state and jumping to C handler takes 15 instructions. We add an extra
 * branch to a common exit path. So each handler takes up one unique cache line
 * and one shared cache line (assuming 16-byte cache lines).
 */
.macro current_exception_spx elx:req handler:req
    save_volatile_to_stack \elx
    bl \handler
    restore_from_stack_and_return \elx
.endm

/**
 * Saves the volatile registers onto the stack. This currently takes 14
 * instructions, so it can be used in exception handlers with 18 instructions
 * left, 2 of which in the same cache line (assuming a 16-byte cache line).
 *
 * On return, x0 and x1 are initialised to elr_el2 and spsr_el2 respectively,
 * which can be used as the first and second arguments of a subsequent call.
 */
.macro save_volatile_to_stack elx:req
    /* Reserve stack space and save registers x0-x18, x29 & x30. */
    stp x0, x1, [sp, #-(8 * 24)]!
    stp x2, x3, [sp, #8 * 2]
    stp x4, x5, [sp, #8 * 4]
    stp x6, x7, [sp, #8 * 6]
    stp x8, x9, [sp, #8 * 8]
    stp x10, x11, [sp, #8 * 10]
    stp x12, x13, [sp, #8 * 12]
    stp x14, x15, [sp, #8 * 14]
    stp x16, x17, [sp, #8 * 16]
    str x18, [sp, #8 * 18]
    stp x29, x30, [sp, #8 * 20]

    /*
     * Save elr_elx & spsr_elx. This such that we can take nested exception
     * and still be able to unwind.
     */
    mrs x0, elr_\elx
    mrs x1, spsr_\elx
    stp x0, x1, [sp, #8 * 22]
.endm

/**
 * Restores the volatile registers from the stack.

 * Register x0: if false restores elr_elx, if true retains the value of elr_elx.
 * This enables exception handlers to indicate whether they have changed the
 * value of elr_elx (e.g., to skip the faulting instruction).
 */
.macro restore_from_stack_and_return elx:req
    /* Restore registers x2-x18, x29 & x30. */
    ldp x2, x3, [sp, #8 * 2]
    ldp x4, x5, [sp, #8 * 4]
    ldp x6, x7, [sp, #8 * 6]
    ldp x8, x9, [sp, #8 * 8]
    ldp x10, x11, [sp, #8 * 10]
    ldp x12, x13, [sp, #8 * 12]
    ldp x14, x15, [sp, #8 * 14]
    ldp x16, x17, [sp, #8 * 16]
    ldr x18, [sp, #8 * 18]
    ldp x29, x30, [sp, #8 * 20]

    cbnz x0, 1f

    /* Restore register elr_elx using x1 as scratch. */
    ldr x1, [sp, #8 * 22]
    msr elr_\elx, x1

1:
    /* Restore register spsr_elx using x1 as scratch. */
    ldr x1, [sp, #8 * 23]
    msr spsr_\elx, x1

    /* Restore x0 & x1, and release stack space. */
    ldp x0, x1, [sp], #8 * 24

    eret
.endm

    .macro    dcache_line_size  reg, tmp
    mrs    \tmp, ctr_el0
    ubfx    \tmp, \tmp, #16, #4
    mov    \reg, #4
    lsl    \reg, \reg, \tmp
    .endm

    .macro    icache_line_size  reg, tmp
    mrs    \tmp, ctr_el0
    and    \tmp, \tmp, #0xf
    mov    \reg, #4
    lsl    \reg, \reg, \tmp
    .endm

/*
 * This macro can be used for implementing various data cache operations `op`
 */
.macro do_dcache_maintenance_by_mva op
    /* Exit early if size is zero */
    cbz    x1, exit_loop_\op
    dcache_line_size x2, x3
    add    x1, x0, x1
    sub    x3, x2, #1
    bic    x0, x0, x3
loop_\op:
    dc    \op, x0
    add    x0, x0, x2
    cmp    x0, x1
    b.lo    loop_\op
    dsb    sy
exit_loop_\op:
    ret
.endm

.macro do_icache_maintenance_by_mva op
    /* Exit early if size is zero */
    cbz    x1, exit_loop_\op
    icache_line_size x2, x3
    add    x1, x0, x1
    sub    x3, x2, #1
    bic    x0, x0, x3
loop_\op:
    ic    \op, x0
    add    x0, x0, x2
    cmp    x0, x1
    b.lo    loop_\op
    dsb    sy
exit_loop_\op:
    ret
.endm
