/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_interfaces.h"
#include "pal_pl011_uart.h"

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

/**
 *   @brief    - This function directs the input character to the respective driver function
 *   @param    - c      : Input Character
 *   @return   - SUCCESS/FAILURE
**/

uint32_t pal_print_driver(uint8_t c)
{
    pal_driver_uart_pl011_putc(c);
    return PAL_SUCCESS;
}
