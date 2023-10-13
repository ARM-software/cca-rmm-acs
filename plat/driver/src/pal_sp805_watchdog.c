/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_sp805_watchdog.h"
#include "pal_interfaces.h"

static inline void pal_sp805_write_wdog_load(unsigned long base, uint32_t value)
{
    pal_mmio_write32(base + SP805_WDOG_LOAD_OFF, value);
}

static inline void pal_sp805_write_wdog_ctrl(unsigned long base, uint32_t value)
{
    /* Not setting reserved bits */
    pal_mmio_write32(base + SP805_WDOG_CTRL_OFF, value);
}

static inline void pal_sp805_write_wdog_int_clr(unsigned long base, uint32_t value)
{
    pal_mmio_write32(base + SP805_WDOG_INT_CLR_OFF, value);
}

static inline void pal_sp805_write_wdog_lock(unsigned long base, uint32_t value)
{
    pal_mmio_write32(base + SP805_WDOG_LOCK_OFF, value);
}

void pal_driver_sp805_wdog_start(unsigned long base)
{
    /* Unlock to access the watchdog registers */
    pal_sp805_write_wdog_lock(base, SP805_WDOG_UNLOCK_ACCESS);

    /* Write the number of cycles needed */
    pal_sp805_write_wdog_load(base, SP805_WDOG_LOAD_VALUE);

    /* Enable reset interrupt and watchdog interrupt on expiry */
    pal_sp805_write_wdog_ctrl(base,
            SP805_WDOG_CTRL_RESEN | SP805_WDOG_CTRL_INTEN);

    /* Lock registers so that they can't be accidently overwritten */
    pal_sp805_write_wdog_lock(base, 0x0);
}

void pal_driver_sp805_wdog_stop(unsigned long base)
{
    /* Unlock to access the watchdog registers */
    pal_sp805_write_wdog_lock(base, SP805_WDOG_UNLOCK_ACCESS);

    /* Clearing INTEN bit stops the counter */
    pal_sp805_write_wdog_ctrl(base, 0x00);

    /* Lock registers so that they can't be accidently overwritten */
    pal_sp805_write_wdog_lock(base, 0x0);
}

void pal_driver_sp805_wdog_refresh(unsigned long base)
{
    /* Unlock to access the watchdog registers */
    pal_sp805_write_wdog_lock(base, SP805_WDOG_UNLOCK_ACCESS);

    /*
     * Write of any value to WdogIntClr clears interrupt and reloads
     * the counter from the value in WdogLoad Register.
     **/
    pal_sp805_write_wdog_int_clr(base, 1);

    /* Lock registers so that they can't be accidently overwritten */
    pal_sp805_write_wdog_lock(base, 0x0);
}

void pal_driver_ns_wdog_start(uint32_t ms)
{
    pal_mmio_write32(PLATFORM_NS_WD_BASE + 0x8, ms);
    pal_mmio_write32(PLATFORM_NS_WD_BASE, 1);
}

void pal_driver_ns_wdog_stop(void)
{
    pal_mmio_write32(PLATFORM_NS_WD_BASE, 0);
}

void pal_driver_sp805_twdog_start(unsigned long base, uint32_t ms)
{
    uint32_t wd_cycles = (ms * ARM_SP805_TWDG_CLK_HZ) / 1000000;

    /* Unlock to access the watchdog registers */
    pal_sp805_write_wdog_lock(base, SP805_WDOG_UNLOCK_ACCESS);

    /* Write the number of cycles needed */
    pal_sp805_write_wdog_load(base, wd_cycles);

    /* Enable reset interrupt and watchdog interrupt on expiry */
    pal_sp805_write_wdog_ctrl(base,
            SP805_WDOG_CTRL_RESEN | SP805_WDOG_CTRL_INTEN);

    /* Lock registers so that they can't be accidently overwritten */
    pal_sp805_write_wdog_lock(base, 0x0);
}
