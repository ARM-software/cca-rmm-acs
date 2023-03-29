/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __PAL_IRQ_H__
#define  __PAL_IRQ_H__

#include <pal_libc.h>
/*
 * SGI sent by the timer management framework to notify CPUs when the system
 * timer fires off
 */
#define IRQ_WAKE_SGI        IRQ_NS_SGI_7

#ifndef __ASSEMBLY__

/* Prototype of a handler function for an IRQ */
typedef int (*handler_irq_t)(void *data);

/* Keep track of the IRQ handler registered for a given SPI */
typedef struct {
    handler_irq_t handler;
} spi_desc;

/* Data associated with the reception of an SGI */
typedef struct {
    /* Interrupt ID of the signaled interrupt */
    unsigned int irq_id;
} sgi_data_t;

/* Keep track of the IRQ handler registered for a spurious interrupt */
typedef handler_irq_t spurious_desc;

/*******************************************************************************
 *  * Used to align variables on the biggest cache line size in the platform.
 *   * This is known only to the platform as it might have a combination of
 *    * integrated and external caches.
 *     ******************************************************************************/
#define CACHE_WRITEBACK_SHIFT_PAL       6
#define CACHE_WRITEBACK_GRANULE_PAL     (1U << CACHE_WRITEBACK_SHIFT_PAL)

/*
 * PPIs and SGIs are interrupts that are private to a GIC CPU interface. These
 * interrupts are banked in the GIC Distributor. Therefore, each CPU can
 * set up a different IRQ handler for a given PPI/SGI.
 *
 * So we define a data structure representing an IRQ handler aligned on the
 * size of a cache line. This guarantees that in an array of these, each element
 * is loaded in a separate cache line. This allows efficient concurrent
 * manipulation of these elements on different CPUs.
 */
typedef struct {
    handler_irq_t handler;
} __aligned(CACHE_WRITEBACK_GRANULE_PAL) irq_handler_banked_t;

/** IRQ/FIQ pin used for signaling a virtual interrupt. */
enum interrupt_pin {
    INTERRUPT_TYPE_IRQ,
    INTERRUPT_TYPE_FIQ,
};

typedef irq_handler_banked_t ppi_desc;
typedef irq_handler_banked_t sgi_desc;

void pal_irq_setup(void);

/*
 * Generic handler called upon reception of an IRQ.
 *
 * This function acknowledges the interrupt, calls the user-defined handler
 * if one has been registered then marks the processing of the interrupt as
 * complete.
 */
int pal_irq_handler_dispatcher(void);

/*
 * Enable interrupt #irq_num for the calling core.
 */
void pal_irq_enable(unsigned int irq_num, uint8_t irq_priority);

/*
 * Enable secure interrupt #irq_num for the calling core.
 */
void pal_configure_secure_irq_enable(unsigned int irq_num);

/*
 * Disable interrupt #irq_num for the calling core.
 */
void pal_irq_disable(unsigned int irq_num);

/*
 * Register an interrupt handler for a given interrupt number.
 * Will fail if there is already an interrupt handler registered for the same
 * interrupt.
 *
 * Return 0 on success, a negative value otherwise.
 */
int pal_irq_register_handler(unsigned int num, handler_irq_t irq_handler);

/*
 * Unregister an interrupt handler for a given interrupt number.
 * Will fail if there is no interrupt handler registered for that interrupt.
 *
 * Return 0 on success, a negative value otherwise.
 */
int pal_irq_unregister_handler(unsigned int irq_num);

void pal_send_sgi(unsigned int sgi_id, unsigned int core_pos);
uint32_t pal_get_irq_num(void);
void pal_gic_end_of_intr(unsigned int irq_num);
#endif /* __ASSEMBLY__ */

#endif /* __PAL_IRQ_H__ */
