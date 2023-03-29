/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <pal_arch.h>
#include <pal_arch_helpers.h>
#include <pal_gic_common.h>
#include <pal_gic_v3.h>
#include <pal_arm_gic.h>

void arm_gic_enable_interrupts_local(void)
{
    gicv3_enable_cpuif();

}

void arm_gic_setup_local(void)
{
    gicv3_probe_redistif_addr();
    gicv3_setup_cpuif();
}

void arm_gic_disable_interrupts_local(void)
{
    gicv3_disable_cpuif();
}

void arm_gic_save_context_local(void)
{
    gicv3_save_cpuif_context();
}

void arm_gic_restore_context_local(void)
{
    gicv3_restore_cpuif_context();
}

void arm_gic_save_context_global(void)
{
    gicv3_save_sgi_ppi_context();
}

void arm_gic_restore_context_global(void)
{
    gicv3_setup_distif();
    gicv3_restore_sgi_ppi_context();
}

void arm_gic_setup_global(void)
{
    gicv3_setup_distif();
}

unsigned int arm_gic_get_intr_priority(unsigned int num)
{
    return gicv3_get_ipriorityr(num);
}

void arm_gic_set_intr_priority(unsigned int num,
                unsigned int priority)
{
    gicv3_set_ipriorityr(num, priority);
}

void arm_gic_send_sgi(unsigned int sgi_id, unsigned int core_pos)
{
    gicv3_send_sgi(sgi_id, core_pos);
}

void arm_gic_set_intr_target(unsigned int num, unsigned int core_pos)
{
    gicv3_set_intr_route(num, core_pos);
}

unsigned int arm_gic_intr_enabled(unsigned int num)
{
    return gicv3_get_isenabler(num) != 0;
}

void arm_gic_intr_enable(unsigned int num)
{
    gicv3_set_isenabler(num);
}

void arm_configure_secure_gic_intr_enable(unsigned int num)
{
    gicv3_configure_secure_gic_intr_enable(num);
}

void arm_gic_intr_disable(unsigned int num)
{
    gicv3_set_icenabler(num);
}

unsigned int arm_gic_intr_ack(unsigned int *raw_iar)
{
    assert(raw_iar);

    *raw_iar = gicv3_acknowledge_interrupt();
    return *raw_iar;
}

unsigned int arm_gic_is_intr_pending(unsigned int num)
{
    return gicv3_get_ispendr(num);
}

void arm_gic_intr_clear(unsigned int num)
{
    gicv3_set_icpendr(num);
}

void arm_gic_end_of_intr(unsigned int raw_iar)
{
    gicv3_end_of_interrupt(raw_iar);
}

void arm_gic_init(uintptr_t gicd_base, uintptr_t gicr_base)
{
    gicv3_init(gicr_base, gicd_base);
    pal_printf("GICv3 mode detected\n", 0, 0);
    arm_gic_setup_global();
    arm_gic_setup_local();
    pal_printf("GICv3 local and global initialisation done\n", 0, 0);

}
