/*
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_timer.h"
#include "val_irq.h"
#include "pal_interfaces.h"
#include "val.h"

/**
 *   @brief   This API disables the EL1 Architecture physical timer
 *   @param   void
 *   @return  void
**/
void val_disable_phy_timer_el1(void)
{
    uint64_t timer_ctrl_reg;

    /* Disable timer */
    timer_ctrl_reg = read_cntp_ctl_el0();
    timer_ctrl_reg |= ARM_ARCH_TIMER_IMASK;
    timer_ctrl_reg &= (~ARM_ARCH_TIMER_ENABLE);
    write_cntp_ctl_el0(timer_ctrl_reg);
}

/**
 *   @brief   This API programs the el1 phy timer with the input timeout value.
 *   @param   timeout   - clock ticks after which an interrupt is generated.
 *   @param   irq_mask  - Interrupt mask bit
 *   @return  void
**/
void val_timer_set_phy_el1(uint64_t timeout, bool irq_mask)
{
    uint64_t cval, freq;
    uint64_t timer_ctrl_reg;

    /* Disable timer */
    val_disable_phy_timer_el1();

    /* Program the timer */
    cval = syscounter_read();
    freq = (uint32_t)read_cntfrq_el0();
    cval += (freq * timeout) / 1000000000;
    write_cntp_cval_el0(cval);

    /* Enable the timer */
    timer_ctrl_reg = read_cntp_ctl_el0();
    if (irq_mask)
    {
        timer_ctrl_reg |= (ARM_ARCH_TIMER_IMASK);
    } else {
        timer_ctrl_reg &= (~ARM_ARCH_TIMER_IMASK);
    }
    timer_ctrl_reg |= ARM_ARCH_TIMER_ENABLE;
    write_cntp_ctl_el0(timer_ctrl_reg);
}

/**
 *   @brief   This API disables the EL1 Architecture virt timer
 *   @param   void
 *   @return  void
**/
void val_disable_virt_timer_el1(void)
{
    uint64_t timer_ctrl_reg;

    /* Disable timer */
    timer_ctrl_reg = read_cntv_ctl_el0();
    timer_ctrl_reg |= ARM_ARCH_TIMER_IMASK;
    timer_ctrl_reg &= (~ARM_ARCH_TIMER_ENABLE);
    write_cntv_ctl_el0(timer_ctrl_reg);
}

/**
 *   @brief   This API programs the el1 virt timer with the input timeout value.
 *   @param   timeout   - clock ticks after which an interrupt is generated.
 *   @return  void
**/
void val_timer_set_virt_el1(uint64_t timeout)
{
    uint64_t cval, freq;
    uint64_t timer_ctrl_reg;

    /* Disable timer */
    val_disable_virt_timer_el1();

    /* program the timer */
    cval = syscounter_read();
    freq = (uint32_t)read_cntfrq_el0();
    cval += (freq * timeout) / 1000000;
    write_cntv_cval_el0(cval);

    /* Enable the timer */
    timer_ctrl_reg = read_cntv_ctl_el0();
    timer_ctrl_reg &= (~ARM_ARCH_TIMER_IMASK);
    timer_ctrl_reg |= ARM_ARCH_TIMER_ENABLE;
    write_cntv_ctl_el0(timer_ctrl_reg);
}

/**
 *   @brief   This API disables the EL2 Architecture phy timer
 *   @param   void
 *   @return  void
**/
void val_disable_phy_timer_el2(void)
{
    uint64_t timer_ctrl_reg;

    /* Disable timer */
    timer_ctrl_reg = read_cnthp_ctl_el2();
    timer_ctrl_reg |= ARM_ARCH_TIMER_IMASK;
    timer_ctrl_reg &= (~ARM_ARCH_TIMER_ENABLE);
    write_cnthp_ctl_el2(timer_ctrl_reg);
}

/**
 *   @brief   This API programs the el2 phys timer with the input timeout value.
 *   @param   timeout   - clock ticks after which an interrupt is generated.
 *   @return  void
**/
void val_timer_set_phy_el2(uint64_t timeout)
{
    uint64_t cval;
    uint64_t freq;
    uint64_t timer_ctrl_reg;

    /* Disable timer */
    val_disable_phy_timer_el2();

    /* Program the timer */
    cval = syscounter_read();
    freq = (uint32_t)read_cntfrq_el0();
    cval += (freq * timeout) / 1000000000;
    write_cnthp_cval_el2(cval);

    /* Enable the timer */
    timer_ctrl_reg = read_cnthp_ctl_el2();
    timer_ctrl_reg &= (~ARM_ARCH_TIMER_IMASK);
    timer_ctrl_reg |= ARM_ARCH_TIMER_ENABLE;
    write_cnthp_ctl_el2(timer_ctrl_reg);
}

/**
 *   @brief   This API Executes software loop based wait for ms.
 *   @param   ms   - wait loop timeout value
 *   @return  void
**/
static inline void val_wait_loop(uint64_t ms)
{
    uint64_t timeout = ms * ITERATIONS_PER_MS;
    uint64_t loop;
    volatile uint64_t count = 0; /* to prevent optimization*/

    for (loop = 0; loop < timeout; loop++) {
        /* Wait */
        count++;
    }
}

/**
 *   @brief   This API Executes software loop based wait for ms.
 *   @param   ms   - wait loop timeout value
 *   @return  void
**/
void val_sp_sleep(uint64_t ms)
{
    val_wait_loop(ms);
}

/**
 *   @brief   This API reads the system counter value.
 *   @param   none.
 *   @return  counter value.
**/
uint64_t val_read_cntpct_el0(void)
{
    return syscounter_read();
}

/**
 *   @brief   This API reads the system counter frequency.
 *   @param   none.
 *   @return  counter frequency value.
**/
uint64_t val_read_cntfrq_el0(void)
{
    return read_cntfrq_el0();
}

/**
 *   @brief   This API reads waits for ms using the arch timer.
 *   @param   ms   - wait time in milli seconds.
 *   @return  returns the wait time as read from system counter.
**/
uint64_t val_sleep_elapsed_time(uint64_t ms)
{
    uint64_t timer_freq = read_cntfrq_el0();
    uint64_t time1 = virtualcounter_read();
    volatile uint64_t time2 = time1;

    while ((time2 - time1) < ((ms * timer_freq) / 1000U)) {
        time2 = virtualcounter_read();
    }

    return ((time2 - time1) * 1000) / timer_freq;
}
