/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_interfaces.h"

uint32_t pal_terminate_simulation(void)
{
   asm volatile("wfi" : : : "memory");
   return PAL_SUCCESS;
}
