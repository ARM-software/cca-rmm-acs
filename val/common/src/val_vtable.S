/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_macros.S"

  .section .text.vtable, "ax"
  .align 12

.global vector_table
vector_table:

// ------------------------------------------------------------
// Current EL with SP0
// ------------------------------------------------------------
  .balign 128
sync_current_el_sp0:
  B        .                    //        Synchronous

  .balign 128
irq_current_el_sp0:
  B        .                    //        IRQ

  .balign 128
fiq_current_el_sp0:
  B        .                    //        FIQ

  .balign 128
serror_current_el_sp0:
  B        .                    //        SError

// ------------------------------------------------------------
// Current EL with SPx
// ------------------------------------------------------------

  .balign 128
sync_current_el_spx:
  current_exception_spx el1 val_sync_exception_current

  .balign 128
irq_current_el_spx:
  current_exception_spx el2 val_irq_current

  .balign 128
fiq_current_el_spx:
  current_exception_spx el2 val_irq_current

  .balign 128
serror_current_el_spx:
  B        .                    //        SError

// ------------------------------------------------------------
// Lower EL using AArch64
// ------------------------------------------------------------

  .balign 128
sync_lower_el_aarch64:
   B        .

  .balign 128
irq_lower_el_aarch64:
  B        .                    //        IRQ

  .balign 128
fiq_lower_el_aarch64:
  B        .                    //        FIQ

  .balign 128
serror_lower_el_aarch64:
  B        .                    //        SError

// ------------------------------------------------------------
// Lower EL using AArch32
// ------------------------------------------------------------

  .balign 128
sync_lower_el_aarch32:
   B        .

  .balign 128
irq_lower_el_aarch32:
  B        .                    //        IRQ

  .balign 128
fiq_lower_el_aarch32:
  B        .                    //        FIQ

  .balign 128
serror_lower_el_aarch32:
  B        .                    //        SError

// ------------------------------------------------------------
// Translation tables
// ------------------------------------------------------------
// 4KB Granule - .align 12
// 16KB Granule - .align 14
// 64KB Granule - .align 16

  .section .bss.tt, "aw"
  .align 12
  .global tt_l0_base
tt_l0_base:
  .fill  4096

  .align 12
  .global tt_l1_base
tt_l1_base:
  .fill 4096

  .align 12
 // Allocate space for 4 contiguous L2 tables
  .global tt_l2_base_1
tt_l2_base_1:
  .fill 16384

  .align 12
 // Allocate space for 4 contiguous L2 tables
  .global tt_l2_base_2
tt_l2_base_2:
  .fill 16384

  .align 12
 // Allocate space for 4 contiguous L2 tables
  .global tt_l2_base_3
tt_l2_base_3:
  .fill 16384

  .align 12
 // Allocate space for 4 contiguous L2 tables
  .global tt_l2_base_4
tt_l2_base_4:
  .fill 16384

  .align 12
 // Allocate space for 4 contiguous L2 tables
  .global tt_l2_base_5
tt_l2_base_5:
  .fill 16384

  .align 12
 // Allocate space for 4 contiguous L2 tables
  .global tt_l2_base_6
tt_l2_base_6:
  .fill 16384

   .align 12
 // Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_1
tt_l3_base_1:
  .fill 16384

  .align 12
// Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_2
tt_l3_base_2:
  .fill 16384

  .align 12
// Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_3
tt_l3_base_3:
  .fill 16384

    .align 12
// Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_4
tt_l3_base_4:
  .fill 16384

    .align 12
// Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_5
tt_l3_base_5:
  .fill 16384

    .align 12
// Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_6
tt_l3_base_6:
  .fill 16384

    .align 12
// Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_7
tt_l3_base_7:
  .fill 16384

    .align 12
// Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_8
tt_l3_base_8:
  .fill 16384

    .align 12
// Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_9
tt_l3_base_9:
  .fill 16384

    .align 12
// Allocate space for 6 contiguous L3 tables
  .global tt_l3_base_10
tt_l3_base_10:
  .fill 16384
