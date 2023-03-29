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
#include <pal_mmio.h>

/*******************************************************************************
 * GIC Distributor interface accessors for reading entire registers
 ******************************************************************************/

unsigned int gicd_read_isenabler(uintptr_t base, unsigned int interrupt_id)
{
    unsigned int n = interrupt_id >> ISENABLER_SHIFT;

    return pal_mmio_read32(base + GICD_ISENABLER + (n << 2));
}

unsigned int gicd_read_icenabler(uintptr_t base, unsigned int interrupt_id)
{
    unsigned int n = interrupt_id >> ICENABLER_SHIFT;

    return pal_mmio_read32(base + GICD_ICENABLER + (n << 2));
}

unsigned int gicd_read_ispendr(uintptr_t base, unsigned int interrupt_id)
{
    unsigned int n = interrupt_id >> ISPENDR_SHIFT;

    return pal_mmio_read32(base + GICD_ISPENDR + (n << 2));
}

unsigned int gicd_read_icpendr(uintptr_t base, unsigned int interrupt_id)
{
    unsigned int n = interrupt_id >> ICPENDR_SHIFT;

    return pal_mmio_read32(base + GICD_ICPENDR + (n << 2));
}

unsigned int gicd_read_isactiver(uintptr_t base, unsigned int interrupt_id)
{
    unsigned int n = interrupt_id >> ISACTIVER_SHIFT;

    return pal_mmio_read32(base + GICD_ISACTIVER + (n << 2));
}

unsigned int gicd_read_icactiver(uintptr_t base, unsigned int interrupt_id)
{
    unsigned int n = interrupt_id >> ICACTIVER_SHIFT;

    return pal_mmio_read32(base + GICD_ICACTIVER + (n << 2));
}

unsigned int gicd_read_ipriorityr(uintptr_t base, unsigned int interrupt_id)
{
    unsigned int n = interrupt_id >> IPRIORITYR_SHIFT;

    return pal_mmio_read32(base + GICD_IPRIORITYR + (n << 2));
}

unsigned int gicd_read_icfgr(uintptr_t base, unsigned int interrupt_id)
{
    unsigned int n = interrupt_id >> ICFGR_SHIFT;

    return pal_mmio_read32(base + GICD_ICFGR + (n << 2));
}

/*******************************************************************************
 * GIC Distributor interface accessors for writing entire registers
 ******************************************************************************/

void gicd_write_isenabler(uintptr_t base,
                unsigned int interrupt_id, unsigned int val)
{
    unsigned int n = interrupt_id >> ISENABLER_SHIFT;

    pal_mmio_write32(base + GICD_ISENABLER + (n << 2), val);
}

void gicd_write_icenabler(uintptr_t base,
                unsigned int interrupt_id, unsigned int val)
{
    unsigned int n = interrupt_id >> ICENABLER_SHIFT;

    pal_mmio_write32(base + GICD_ICENABLER + (n << 2), val);
}

void gicd_write_ispendr(uintptr_t base,
                unsigned int interrupt_id, unsigned int val)
{
    unsigned int n = interrupt_id >> ISPENDR_SHIFT;

    pal_mmio_write32(base + GICD_ISPENDR + (n << 2), val);
}

void gicd_write_icpendr(uintptr_t base,
                unsigned int interrupt_id, unsigned int val)
{
    unsigned int n = interrupt_id >> ICPENDR_SHIFT;

    pal_mmio_write32(base + GICD_ICPENDR + (n << 2), val);
}

void gicd_write_isactiver(uintptr_t base,
                unsigned int interrupt_id, unsigned int val)
{
    unsigned int n = interrupt_id >> ISACTIVER_SHIFT;

    pal_mmio_write32(base + GICD_ISACTIVER + (n << 2), val);
}

void gicd_write_icactiver(uintptr_t base,
                unsigned int interrupt_id, unsigned int val)
{
    unsigned int n = interrupt_id >> ICACTIVER_SHIFT;

    pal_mmio_write32(base + GICD_ICACTIVER + (n << 2), val);
}

void gicd_write_ipriorityr(uintptr_t base,
                unsigned int interrupt_id, unsigned int val)
{
    unsigned int n = interrupt_id >> IPRIORITYR_SHIFT;

    pal_mmio_write32(base + GICD_IPRIORITYR + (n << 2), val);
}

void gicd_write_icfgr(uintptr_t base,
                unsigned int interrupt_id, unsigned int val)
{
    unsigned int n = interrupt_id >> ICFGR_SHIFT;

    pal_mmio_write32(base + GICD_ICFGR + (n << 2), val);
}

/*******************************************************************************
 * GIC Distributor interface accessors for individual interrupt manipulation
 ******************************************************************************/
unsigned int gicd_get_isenabler(uintptr_t base, unsigned int interrupt_id)
{
    unsigned int bit_num = interrupt_id & ((1 << ISENABLER_SHIFT) - 1);

    return gicd_read_isenabler(base, interrupt_id) & (1U << bit_num);
}

void gicd_set_isenabler(uintptr_t base, unsigned int interrupt_id)
{
    unsigned int bit_num = interrupt_id & ((1 << ISENABLER_SHIFT) - 1);

    gicd_write_isenabler(base, interrupt_id, (1U << bit_num));
}

void gicd_set_icenabler(uintptr_t base, unsigned int interrupt_id)
{
    unsigned int bit_num = interrupt_id & ((1 << ICENABLER_SHIFT) - 1);

    gicd_write_icenabler(base, interrupt_id, (1U << bit_num));
}

void gicd_set_ispendr(uintptr_t base, unsigned int interrupt_id)
{
    unsigned int bit_num = interrupt_id & ((1 << ISPENDR_SHIFT) - 1);

    gicd_write_ispendr(base, interrupt_id, (1U << bit_num));
}

void gicd_set_icpendr(uintptr_t base, unsigned int interrupt_id)
{
    unsigned int bit_num = interrupt_id & ((1 << ICPENDR_SHIFT) - 1);

    gicd_write_icpendr(base, interrupt_id, (1U << bit_num));
}

void gicd_set_isactiver(uintptr_t base, unsigned int interrupt_id)
{
    unsigned int bit_num = interrupt_id & ((1 << ISACTIVER_SHIFT) - 1);

    gicd_write_isactiver(base, interrupt_id, (1U << bit_num));
}

void gicd_set_icactiver(uintptr_t base, unsigned int interrupt_id)
{
    unsigned int bit_num = interrupt_id & ((1 << ICACTIVER_SHIFT) - 1);

    gicd_write_icactiver(base, interrupt_id, (1U << bit_num));
}

unsigned int gicd_get_ipriorityr(uintptr_t base, unsigned int interrupt_id)
{
    return gicd_read_ipriorityr(base, interrupt_id) & GIC_PRI_MASK;
}

void gicd_set_ipriorityr(uintptr_t base, unsigned int interrupt_id,
                unsigned int priority)
{
    pal_mmio_write8(base + GICD_IPRIORITYR + interrupt_id,
            priority & GIC_PRI_MASK);
}

unsigned int is_gicv3_mode(void)
{
    /* Check if GICv3 system register available */
#ifdef __aarch64__
    if (!(read_id_aa64pfr0_el1() & (ID_AA64PFR0_GIC_MASK << ID_AA64PFR0_GIC_SHIFT)))
        return 0;
#else
    if (!(read_id_pfr1() & (ID_PFR1_GIC_MASK << ID_PFR1_GIC_SHIFT)))
        return 0;
#endif

    /* Check whether the system register interface is enabled */
    return !!is_sre_enabled();
}
