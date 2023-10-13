/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <pal_arch_helpers.h>
#include <pal_arm_gic.h>
#include <pal_interfaces.h>
#include <platform.h>
#include <pal_gic_common.h>

/*
 * On FVP, consider that the last SPI is the Trusted Random Number Generator
 * interrupt.
 */
#define PLAT_MAX_SPI_OFFSET_ID      107

#define IS_PLAT_SPI(irq_num)                        \
    (((irq_num) >= MIN_SPI_ID) &&                    \
     ((irq_num) <= MIN_SPI_ID + PLAT_MAX_SPI_OFFSET_ID))

static spi_desc spi_desc_table[PLAT_MAX_SPI_OFFSET_ID + 1];
static ppi_desc ppi_desc_table[PLATFORM_CPU_COUNT][
                (MAX_PPI_ID + 1) - MIN_PPI_ID];
static sgi_desc sgi_desc_table[PLATFORM_CPU_COUNT][MAX_SGI_ID + 1];
static spurious_desc spurious_desc_handler;

extern uint64_t security_state;

/*
 * For a given SPI, the associated IRQ handler is common to all CPUs.
 * Therefore, we need a lock to prevent simultaneous updates.
 *
 * We use one lock for all SPIs. This will make it impossible to update
 * different SPIs' handlers at the same time (although it would be fine) but it
 * saves memory. Updating an SPI handler shouldn't occur that often anyway so we
 * shouldn't suffer from this restriction too much.
 */
static s_lock_t spi_lock;

static handler_irq_t *get_irq_handler(unsigned int irq_num)
{
    if (IS_PLAT_SPI(irq_num))
        return &spi_desc_table[irq_num - MIN_SPI_ID].handler;

    unsigned int mpid = (uint32_t)read_mpidr_el1();
    unsigned int linear_id = platform_get_core_pos(mpid);

    if (IS_PPI(irq_num))
        return &ppi_desc_table[linear_id][irq_num - MIN_PPI_ID].handler;

    if (IS_SGI(irq_num))
        return &sgi_desc_table[linear_id][irq_num - MIN_SGI_ID].handler;

    /*
     * The only possibility is for it to be a spurious
     * interrupt.
     */
    assert(irq_num == GIC_SPURIOUS_INTERRUPT);
    return &spurious_desc_handler;
}

void pal_send_sgi(unsigned int sgi_id, unsigned int core_pos)
{
    assert(IS_SGI(sgi_id));

    /*
     * Ensure that all memory accesses prior to sending the SGI are
     * completed.
     */
    dsbish();

    /*
     * Don't send interrupts to CPUs that are powering down. That would be a
     * violation of the PSCI CPU_OFF caller responsibilities. The PSCI
     * specification explicitly says:
     * "Asynchronous wake-ups on a core that has been switched off through a
     * PSCI CPU_OFF call results in an erroneous state. When this erroneous
     * state is observed, it is IMPLEMENTATION DEFINED how the PSCI
     * implementation reacts."
     */
    //assert(pal_is_core_pos_online(core_pos));
    arm_gic_send_sgi(sgi_id, core_pos);
    enable_irq();
}

void pal_irq_enable(unsigned int irq_num, uint8_t irq_priority)
{
    disable_irq();
    if (IS_PLAT_SPI(irq_num)) {
        /*
         * Instruct the GIC Distributor to forward the interrupt to
         * the calling core
         */
        arm_gic_set_intr_target(irq_num, platform_get_core_pos(read_mpidr_el1()));
    }

    arm_gic_set_intr_priority(irq_num, irq_priority);
    arm_gic_intr_enable(irq_num);

    enable_irq();
    enable_fiq();
}

void pal_configure_secure_irq_enable(unsigned int irq_num)
{
  arm_configure_secure_gic_intr_enable(irq_num);
}

void pal_irq_disable(unsigned int irq_num)
{
    /* Disable the interrupt */
    arm_gic_intr_disable(irq_num);

}

#define HANDLER_VALID(handler, expect_handler)        \
    ((expect_handler) ? ((handler) != NULL) : ((handler) == NULL))

static int pal_irq_update_handler(unsigned int irq_num,
                   handler_irq_t irq_handler,
                   bool expect_handler)
{
    handler_irq_t *cur_handler;
    int ret = -1;

    cur_handler = get_irq_handler(irq_num);
    if (IS_PLAT_SPI(irq_num))
        pal_spin_lock(&spi_lock);

    /*
     * Update the IRQ handler, if the current handler is in the expected
     * state
     */
    assert(HANDLER_VALID(*cur_handler, expect_handler));
    if (HANDLER_VALID(*cur_handler, expect_handler)) {
        *cur_handler = irq_handler;
        ret = 0;
    }

    if (IS_PLAT_SPI(irq_num))
        pal_spin_unlock(&spi_lock);

    return ret;
}

int pal_irq_register_handler(unsigned int irq_num, handler_irq_t irq_handler)
{
    int ret;

    ret = pal_irq_update_handler(irq_num, irq_handler, false);

    return ret;
}

int pal_irq_unregister_handler(unsigned int irq_num)
{
    int ret;

    ret = pal_irq_update_handler(irq_num, NULL, true);

    return ret;
}

uint32_t pal_get_irq_num(void)
{
    unsigned int raw_iar;

    if (security_state == 2)
    {
        return (uint32_t)read_icv_iar1_el1();
    } else {
        return arm_gic_intr_ack(&raw_iar);
    }
}

void pal_gic_end_of_intr(unsigned int irq_num)
{
     if (security_state == 2)
    {
        write_icv_eoir1_el1(irq_num);
    } else {
        arm_gic_end_of_intr(irq_num);
    }
}

int pal_irq_handler_dispatcher(void)
{
    unsigned int irq_num;
    sgi_data_t sgi_data;
    handler_irq_t *handler;
    void *irq_data = NULL;
    /* Acknowledge the interrupt */
    unsigned int raw_iar;

    if (security_state == 2)
    {
        irq_num = (uint32_t)read_icv_iar1_el1();
    } else {
        irq_num = arm_gic_intr_ack(&raw_iar);
    }

    handler = get_irq_handler(irq_num);
    if (IS_PLAT_SPI(irq_num)) {
        irq_data = &irq_num;
    } else if (IS_PPI(irq_num)) {
        irq_data = &irq_num;
    } else if (IS_SGI(irq_num)) {
        sgi_data.irq_id = irq_num;
        irq_data = &sgi_data;
    }

    if (*handler != NULL) {
        (*handler)(irq_data);
    } else {
        return PAL_ERROR;
    }

    if (security_state != 2)
    {
        /* Mark the processing of the interrupt as complete */
        if (irq_num != GIC_SPURIOUS_INTERRUPT)
            arm_gic_end_of_intr(raw_iar);
    }

    return PAL_SUCCESS;
}

void pal_irq_setup(void)
{
    if (security_state == 2)
    {
        enable_irq();
        enable_fiq();
        write_icc_pmr_el1(0xff);
        write_icc_igrpen1_el1(read_icc_igrpen1_el1() | 0x1);
    } else {
        pal_printf("GIC Initialisation started \n", 0, 0);
        arm_gic_init(GICD_BASE, GICR_BASE);
        pal_memset(spi_desc_table, 0, sizeof(spi_desc_table));
        pal_memset(ppi_desc_table, 0, sizeof(ppi_desc_table));
        pal_memset(sgi_desc_table, 0, sizeof(sgi_desc_table));
        pal_memset(&spurious_desc_handler, 0, sizeof(spurious_desc_handler));
        pal_init_spinlock(&spi_lock);
        pal_printf("GIC Initialisation completed \n", 0, 0);
    }
}
