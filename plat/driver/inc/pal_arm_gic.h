/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __ARM_GIC_H__
#define __ARM_GIC_H__

#include <stdint.h>

/***************************************************************************
 * Defines and prototypes for ARM GIC driver.
 **************************************************************************/
#define MAX_SGIS        16
#define MIN_SGI_ID        0
#define MAX_SGI_ID        15
#define MIN_PPI_ID        16
#define MAX_PPI_ID        31
#define MIN_SPI_ID        32
#define MAX_SPI_ID        1019

#define IS_SGI(irq_num)                            \
    (((irq_num) <= MAX_SGI_ID))

#define IS_PPI(irq_num)                            \
    (((irq_num) >= MIN_PPI_ID) && ((irq_num) <= MAX_PPI_ID))

#define IS_SPI(irq_num)                            \
    (((irq_num) >= MIN_SPI_ID) && ((irq_num) <= MAX_SPI_ID))

#define IS_VALID_INTR_ID(irq_num)                    \
    (((irq_num) <= MAX_SPI_ID))

#define GIC_HIGHEST_NS_PRIORITY    0
#define GIC_LOWEST_NS_PRIORITY    254 /* 255 would disable an interrupt */
#define GIC_SPURIOUS_INTERRUPT    1023

/******************************************************************************
 * Setup the global GIC interface. In case of GICv2, it would be the GIC
 * Distributor and in case of GICv3 it would be GIC Distributor and
 * Re-distributor.
 *****************************************************************************/
void arm_gic_setup_global(void);

/******************************************************************************
 * Setup the GIC interface local to the CPU
 *****************************************************************************/
void arm_gic_setup_local(void);

/******************************************************************************
 * Disable interrupts for this local CPU
 *****************************************************************************/
void arm_gic_disable_interrupts_local(void);

/******************************************************************************
 * Enable interrupts for this local CPU
 *****************************************************************************/
void arm_gic_enable_interrupts_local(void);

/******************************************************************************
 * Send SGI with ID `sgi_id` to a core with index `core_pos`.
 *****************************************************************************/
void arm_gic_send_sgi(unsigned int sgi_id, unsigned int core_pos);

/******************************************************************************
 * Set the interrupt target of interrupt ID `num` to a core with index
 * `core_pos`
 *****************************************************************************/
void arm_gic_set_intr_target(unsigned int num, unsigned int core_pos);

/******************************************************************************
 * Get the priority of the interrupt ID `num`.
 *****************************************************************************/
unsigned int arm_gic_get_intr_priority(unsigned int num);

/******************************************************************************
 * Set the priority of the interrupt ID `num` to `priority`.
 *****************************************************************************/
void arm_gic_set_intr_priority(unsigned int num, unsigned int priority);

/******************************************************************************
 * Check if the interrupt ID `num` is enabled
 *****************************************************************************/
unsigned int arm_gic_intr_enabled(unsigned int num);

/******************************************************************************
 * Enable the interrupt ID `num`
 *****************************************************************************/
void arm_gic_intr_enable(unsigned int num);

/******************************************************************************
 * Enable the secure interrupt ID `num`
 *****************************************************************************/
void arm_configure_secure_gic_intr_enable(unsigned int num);

/******************************************************************************
 * Disable the interrupt ID `num`
 *****************************************************************************/
void arm_gic_intr_disable(unsigned int num);

/******************************************************************************
 * Acknowledge the highest pending interrupt. Return the interrupt ID of the
 * acknowledged interrupt. The raw interrupt acknowledge register value will
 * be populated in `raw_iar`.
 *****************************************************************************/
unsigned int arm_gic_intr_ack(unsigned int *raw_iar);

/******************************************************************************
 * Signal the end of interrupt processing of a interrupt. The raw interrupt
 * acknowledge register value returned by arm_gic_intr_ack() should be passed
 * as argument to this function.
 *****************************************************************************/
void arm_gic_end_of_intr(unsigned int raw_iar);

/******************************************************************************
 * Check if the interrupt with ID `num` is pending at the GIC. Returns 1 if
 * interrupt is pending else returns 0.
 *****************************************************************************/
unsigned int arm_gic_is_intr_pending(unsigned int num);

/******************************************************************************
 * Clear the pending status of the interrupt with ID `num` at the GIC.
 *****************************************************************************/
void arm_gic_intr_clear(unsigned int num);

/******************************************************************************
 * Initialize the GIC Driver. This function will detect the GIC Architecture
 * present on the system and initialize the appropriate driver. The
 * `gicr_base` argument will be ignored on GICv2 systems.
 *****************************************************************************/
void arm_gic_init(uintptr_t gicd_base, uintptr_t gicr_base);

/******************************************************************************
 * Save the GIC context local to this CPU (like GIC CPU Interface) which will
 * be lost when this CPU is powered down.
 *****************************************************************************/
void arm_gic_save_context_local(void);

/******************************************************************************
 * Restore the GIC context local to this CPU ((like GIC CPU Interface) which
 * was lost when this CPU was powered down.
 *****************************************************************************/
void arm_gic_restore_context_local(void);

/******************************************************************************
 * Save the global GIC context when GIC will be powered down (like GIC
 * Distributor and Re-distributor) as a result of system suspend.
 *****************************************************************************/
void arm_gic_save_context_global(void);

/******************************************************************************
 * Restore the global GIC context which was lost as a result of GIC power
 * down (like GIC Distributor and Re-distributor) during system suspend.
 *****************************************************************************/
void arm_gic_restore_context_global(void);

#endif /* __ARM_GIC_H__ */
