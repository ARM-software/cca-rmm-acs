/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_framework.h"
#include "val_irq.h"
#include "pal_irq.h"

/**
 *   @brief    Setting up IRQ configurations
 *   @param    void
 *   @return   void
 **/
void val_irq_setup(void)
{
    pal_irq_setup();
}

/**
 *   @brief    Enables IRQ interrupt
 *   @param    irq_num        - IRQ number for which interrupt to enable
 *   @param    irq_priority   - IRQ priority
 *   @return   void
 **/
void val_irq_enable(uint32_t irq_num, uint8_t irq_priority)
{
    pal_irq_enable(irq_num, irq_priority);
}

/**
 *   @brief    Enables secure IRQ interrupt
 *   @param    irq_num        - IRQ number for which interrupt to enable
 *   @return   void
 **/
void val_configure_secure_irq_enable(uint32_t irq_num)
{
    pal_configure_secure_irq_enable(irq_num);
}

/**
 *   @brief    Disables IRQ interrupt
 *   @param    irq_num        - IRQ number for which interrupt to disable
 *   @return   void
 **/
void val_irq_disable(uint32_t irq_num)
{
    pal_irq_disable(irq_num);
}

/**
 *   @brief    Register IRQ handler
 *   @param    num          - IRQ number for which interrupt to register
 *   @param    irq_handler  -  Function pointer of IRQ handler
 *   @return   SUCCESS(0)/FAILURE
 **/
int val_irq_register_handler(uint32_t num, void *irq_handler)
{
    return pal_irq_register_handler(num, irq_handler);
}

/**
 *   @brief    Unregister IRQ handler
 *   @param    num        - IRQ number for which interrupt to register
 *   @return   SUCCESS(0)/FAILURE
 **/
int val_irq_unregister_handler(uint32_t irq_num)
{
    return pal_irq_unregister_handler(irq_num);
}

/**
 *   @brief    End of IRQ interrupt
 *   @param    irq_num        - IRQ number
 *   @return   void
 **/
void val_gic_end_of_intr(unsigned int irq_num)
{
    pal_gic_end_of_intr(irq_num);
}
