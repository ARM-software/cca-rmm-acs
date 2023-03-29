/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_interfaces.h"
#include "pal_pl011_uart.h"
#include "pal_sp805_watchdog.h"
#include "pal_nvm.h"

#define pal_uart_putc(x) pal_driver_uart_pl011_putc(x)

uint32_t pal_printf(const char *msg, uint64_t data1, uint64_t data2)
{
    uint8_t buffer[16];
    uint64_t j, i = 0;
    uint64_t data = data1;

    for (; *msg != '\0'; ++msg)
    {
        if (*msg == '%')
        {
            ++msg;
            if (*msg == 'l' || *msg == 'L')
                ++msg;

            if (*msg == 'd')
            {
                while (data != 0)
                {
                    j         = data % 10;
                    data      = data / 10;
                    buffer[i] = (uint8_t)(j + 48);
                    i        += 1;
                }
                data = data2;
            } else if (*msg == 'x' || *msg == 'X')
            {
                while (data != 0)
                {
                    j         = data & 0xf;
                    data      = data >> 4;
                    buffer[i] = (uint8_t)(j + ((j > 9) ? 55 : 48));
                    i        += 1;
                }
                data = data2;
            }
            if (i > 0)
            {
                while (i > 0)
                {
                    pal_uart_putc(buffer[--i]);
                }
            } else
            {
                pal_uart_putc(48);
            }
        } else
        {
            pal_uart_putc(*msg);

            if (*msg == '\n')
            {
                pal_uart_putc('\r');
            }
        }
    }
    return PAL_SUCCESS;
}

uint32_t pal_nvm_write(uint32_t offset, void *buffer, size_t size)
{
    return pal_driver_nvm_write(offset, buffer, size);
}

uint32_t pal_nvm_read(uint32_t offset, void *buffer, size_t size)
{
    return pal_driver_nvm_read(offset, buffer, size);
}

uint32_t pal_watchdog_enable(void)
{
    pal_driver_sp805_wdog_start(PLATFORM_WDOG_BASE);
    return PAL_SUCCESS;
}

uint32_t pal_watchdog_disable(void)
{
    pal_driver_sp805_wdog_stop(PLATFORM_WDOG_BASE);
    return PAL_SUCCESS;
}

void pal_ns_wdog_enable(uint32_t ms)
{
    pal_driver_ns_wdog_start(ms);
}

void pal_ns_wdog_disable(void)
{
    pal_driver_ns_wdog_stop();
}

uint32_t pal_twdog_enable(uint32_t ms)
{
    pal_driver_sp805_wdog_refresh(PLATFORM_SP805_TWDOG_BASE);
    pal_driver_sp805_twdog_start(PLATFORM_SP805_TWDOG_BASE, ms);
    return PAL_SUCCESS;
}

uint32_t pal_twdog_disable(void)
{
    pal_driver_sp805_wdog_stop(PLATFORM_SP805_TWDOG_BASE);
    return PAL_SUCCESS;
}