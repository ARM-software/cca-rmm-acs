
/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <val_def.h>

  .section .text.syscalls, "ax"

    .global val_smc_call_asm
val_smc_call_asm:

    /* Mov buffer address into callee saved register
     * and copy the buffer content into X0-X10.
     */
    sub sp,sp,#0x20
    str x21,[sp,#0x10]
    mov x21, x0
    ldr x0, [x21]
    ldr x1, [x21, #8]
    ldr x2, [x21, #16]
    ldr x3, [x21, #24]
    ldr x4, [x21, #32]
    ldr x5, [x21, #40]
    ldr x6, [x21, #48]
    ldr x7, [x21, #56]
    ldr x8, [x21, #64]
    ldr x9, [x21, #72]
    ldr x10, [x21, #80]

    smc     #0

    /* Return values are stored in x0-x10, put them into
     * input buffer structure.
     */
    str x0, [x21]
    str x1, [x21, #8]
    str x2, [x21, #16]
    str x3, [x21, #24]
    str x4, [x21, #32]
    str x5, [x21, #40]
    str x6, [x21, #48]
    str x7, [x21, #56]
    str x8, [x21, #64]
    str x9, [x21, #72]
    str x10, [x21, #80]
    ldr x21,[sp,#0x10]
    add     sp,sp,#0x20
    ret

    .global val_return_to_host_hvc_asm
val_return_to_host_hvc_asm:
    hvc #VAL_SWITCH_TO_HOST
    ret

    .global val_realm_printf_msg_hvc_asm
val_realm_printf_msg_hvc_asm:
    hvc #VAL_REALM_PRINT_MSG
    ret
