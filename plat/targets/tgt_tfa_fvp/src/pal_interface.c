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

/*
 * function interface for verifying the signature of the provided token
 * @parameter toekn : token recived from the platform
 * @return : true/false
 */

__attribute__((weak)) uint32_t pal_verify_signature(__attribute__ ((unused)) uint64_t *token)
{
    return PAL_SUCCESS;
}
