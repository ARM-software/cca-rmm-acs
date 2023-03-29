/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

  .section .text.spinlock, "ax"

    .globl    pal_init_spinlock
    .globl    pal_spin_lock
    .globl    pal_spin_unlock

pal_init_spinlock:
    str    wzr, [x0]
    ret

pal_spin_lock:
    mov    w2, #1
    sevl
l1: wfe
l2: ldaxr   w1, [x0]
    cbnz    w1, l1
    stxr    w1, w2, [x0]
    cbnz    w1, l2
    ret


pal_spin_unlock:
    stlr    wzr, [x0]
    ret
