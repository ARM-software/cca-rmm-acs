/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _PAL_SP805_WATCHDOG_H_
#define _PAL_SP805_WATCHDOG_H_

#include <pal_interfaces.h>

#define SP805_WDOG_BASE          PLATFORM_WDOG_BASE
#define SP805_WDOG_LOAD_VALUE    PLATFORM_WDOG_LOAD_VALUE

/* SP805 register offset */
#define SP805_WDOG_LOAD_OFF        0x000
#define SP805_WDOG_VALUE_0FF       0x004
#define SP805_WDOG_CTRL_OFF        0x008
#define SP805_WDOG_INT_CLR_OFF     0x00c
#define SP805_WDOG_RIS_OFF         0x010
#define SP805_WDOG_MIS_OFF         0x014
#define SP805_WDOG_LOCK_OFF        0xc00
#define SP805_WDOG_ITCR_OFF        0xf00
#define SP805_WDOG_ITOP_OFF        0xf04
#define SP805_WDOG_PERIPH_ID_OFF   0xfe0
#define SP805_WDOG_PCELL_ID_OFF    0xff0

/*
 * Magic word to unlock access to all other watchdog registers, Writing any other
 * value locks them.
 */
#define SP805_WDOG_UNLOCK_ACCESS    0x1ACCE551

/* Register field definitions */
#define SP805_WDOG_CTRL_MASK         0x03
#define SP805_WDOG_CTRL_RESEN        (1 << 1)
#define SP805_WDOG_CTRL_INTEN        (1 << 0)
#define SP805_WDOG_RIS_WDOGRIS       (1 << 0)
#define SP805_WDOG_RIS_MASK          0x1
#define SP805_WDOG_MIS_WDOGMIS       (1 << 0)
#define SP805_WDOG_MIS_MASK          0x1
#define SP805_WDOG_ITCR_MASK         0x1
#define SP805_WDOG_ITOP_MASK         0x3
#define SP805_WDOG_PART_NUM_SHIFT    0
#define SP805_WDOG_PART_NUM_MASK     0xfff
#define SP805_WDOG_DESIGNER_ID_SHIFT 12
#define SP805_WDOG_DESIGNER_ID_MASK  0xff
#define SP805_WDOG_REV_SHIFT         20
#define SP805_WDOG_REV_MASK          0xf
#define SP805_WDOG_CFG_SHIFT         24
#define SP805_WDOG_CFG_MASK          0xff
#define SP805_WDOG_PCELL_ID_SHIFT    0
#define SP805_WDOG_PCELL_ID_MASK     0xff

void pal_driver_sp805_wdog_start(unsigned long base);
void pal_driver_sp805_wdog_stop(unsigned long base);
void pal_driver_sp805_wdog_refresh(unsigned long base);
void pal_driver_ns_wdog_start(uint32_t ms);
void pal_driver_ns_wdog_stop(void);
void pal_driver_sp805_twdog_start(unsigned long base, uint32_t ms);


#endif /* _PAL_SP805_WATCHDOG_H_ */

