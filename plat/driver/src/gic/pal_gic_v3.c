/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <pal_arch.h>
#include <pal_arch_helpers.h>
#include <pal_arm_gic.h>
#include <pal_gic_common.h>
#include <pal_gic_v3.h>
#include <pal_mmio.h>
#include <platform.h>
#include <pal_config_def.h>
#include <pal_interfaces.h>

/* Global variables to store the GIC base addresses */
static uintptr_t gicr_base_addr;
static uintptr_t gicd_base_addr;

#ifdef __aarch64__
#define MPIDR_AFFLVL3_MASK    ((unsigned long long)MPIDR_AFFLVL_MASK << MPIDR_AFF3_SHIFT)
#define gic_typer_affinity_from_mpidr(mpidr)    \
    (((mpidr) & (~MPIDR_AFFLVL3_MASK)) | (((mpidr) & MPIDR_AFFLVL3_MASK) >> 8))
#else
#define gic_typer_affinity_from_mpidr(mpidr)    \
    ((mpidr) & ((MPIDR_AFFLVL_MASK << MPIDR_AFF2_SHIFT) | MPID_MASK))
#endif

/*
 * Data structure to store the GIC per CPU context before entering
 * system suspend. Only the GIC context of first 32 interrupts (SGIs and PPIs)
 * will be saved. The GIC SPI context needs to be restored by the respective
 * drivers.
 */
struct gicv3_pcpu_ctx {
    /* Flag to indicate whether the CPU is suspended */
    unsigned int is_suspended;
    unsigned int icc_igrpen1;
    unsigned int gicr_isenabler;
    unsigned int gicr_ipriorityr[NUM_PCPU_INTR >> IPRIORITYR_SHIFT];
    unsigned int gicr_icfgr;
};

/* Array to store the per-cpu GICv3 context when being suspended.*/
static struct gicv3_pcpu_ctx pcpu_ctx[PLATFORM_CPU_COUNT];

/* Array to store the per-cpu redistributor frame addresses */
static uintptr_t rdist_pcpu_base[PLATFORM_CPU_COUNT];

/*
 * Array to store the mpidr corresponding to each initialized per-CPU
 * redistributor interface.
 */
static unsigned long long mpidr_list[PLATFORM_CPU_COUNT] = {UINT64_MAX};

/******************************************************************************
 * GIC Distributor interface accessors for writing entire registers
 *****************************************************************************/
static void gicd_write_irouter(uintptr_t base,
                unsigned int interrupt_id,
                unsigned long long route)
{
    assert(interrupt_id >= MIN_SPI_ID);
    pal_mmio_write64(base + GICD_IROUTER + (interrupt_id << 3), route);
}

/******************************************************************************
 * GIC Re-distributor interface accessors for writing entire registers
 *****************************************************************************/
static void gicr_write_isenabler0(uintptr_t base, unsigned int val)
{
    pal_mmio_write32(base + GICR_ISENABLER0, val);
}

static void gicr_write_icenabler0(uintptr_t base, unsigned int val)
{
    pal_mmio_write32(base + GICR_ICENABLER0, val);
}

static void gicr_write_icpendr0(uintptr_t base, unsigned int val)
{
    pal_mmio_write32(base + GICR_ICPENDR0, val);
}

static void gicr_write_ipriorityr(uintptr_t base, unsigned int id, unsigned int val)
{
    unsigned n = id >> IPRIORITYR_SHIFT;

    pal_mmio_write32(base + GICR_IPRIORITYR + (n << 2), val);
}

static void gicr_write_icfgr1(uintptr_t base, unsigned int val)
{
    pal_mmio_write32(base + GICR_ICFGR1, val);
}

/******************************************************************************
 * GIC Re-distributor interface accessors for reading entire registers
 *****************************************************************************/
static unsigned long long gicr_read_typer(uintptr_t base)
{
    return pal_mmio_read64(base + GICR_TYPER);
}

static unsigned int gicr_read_icfgr1(uintptr_t base)
{
    return pal_mmio_read32(base + GICR_ICFGR1);
}

static unsigned int gicr_read_isenabler0(uintptr_t base)
{
    return pal_mmio_read32(base + GICR_ISENABLER0);
}

static unsigned int gicr_read_ipriorityr(uintptr_t base, unsigned int id)
{
    unsigned n = id >> IPRIORITYR_SHIFT;

    return pal_mmio_read32(base + GICR_IPRIORITYR + (n << 2));
}

static unsigned int gicr_read_ispendr0(uintptr_t base)
{
    return pal_mmio_read32(base + GICR_ISPENDR0);
}

/******************************************************************************
 * GIC Re-distributor interface accessors for individual interrupt
 * manipulation
 *****************************************************************************/
static unsigned int gicr_get_isenabler0(uintptr_t base,
    unsigned int interrupt_id)
{
    unsigned int bit_num = interrupt_id & ((1 << ISENABLER_SHIFT) - 1);

    return gicr_read_isenabler0(base) & (1U << bit_num);
}

static void gicr_set_isenabler0(uintptr_t base, unsigned int interrupt_id)
{
    unsigned bit_num = interrupt_id & ((1 << ISENABLER_SHIFT) - 1);

    gicr_write_isenabler0(base, (1U << bit_num));
}

static void gicr_set_icenabler0(uintptr_t base, unsigned int interrupt_id)
{
    unsigned bit_num = interrupt_id & ((1 << ISENABLER_SHIFT) - 1);

    gicr_write_icenabler0(base, (1U << bit_num));
}

static void gicr_set_icpendr0(uintptr_t base, unsigned int interrupt_id)
{
    unsigned bit_num = interrupt_id & ((1 << ICPENDR_SHIFT) - 1);

    gicr_write_icpendr0(base, (1U << bit_num));
}

/******************************************************************************
 * GICv3 public driver API
 *****************************************************************************/
void gicv3_enable_cpuif(void)
{
    /* Assert that system register access is enabled */
    assert(IS_IN_EL2() ? (read_icc_sre_el2() & ICC_SRE_SRE_BIT) :
                (read_icc_sre_el1() & ICC_SRE_SRE_BIT));

    /* Enable Group1 non secure interrupts */
    write_icc_igrpen1_el1(read_icc_igrpen1_el1() | IGRPEN1_EL1_ENABLE_BIT);
    isb();
}

void gicv3_setup_cpuif(void)
{
    /* Set the priority mask register to allow all interrupts to trickle in */
    write_icc_pmr_el1(GIC_PRI_MASK);
    isb();
    gicv3_enable_cpuif();
}

void gicv3_disable_cpuif(void)
{
    /* Disable Group1 non secure interrupts */
    write_icc_igrpen1_el1(read_icc_igrpen1_el1() &
            ~IGRPEN1_EL1_ENABLE_BIT);
    isb();
}

void gicv3_save_cpuif_context(void)
{
    unsigned int core_pos = platform_get_core_pos(read_mpidr_el1());

    /* Set the `is_suspended` flag as this core is being suspended. */
    pcpu_ctx[core_pos].is_suspended = 1;
    pcpu_ctx[core_pos].icc_igrpen1 = (unsigned int)read_icc_igrpen1_el1();
}

void gicv3_restore_cpuif_context(void)
{
    unsigned int core_pos = platform_get_core_pos(read_mpidr_el1());

    /* Reset the `is_suspended` flag as this core has resumed from suspend. */
    pcpu_ctx[core_pos].is_suspended = 0;
    write_icc_pmr_el1(GIC_PRI_MASK);
    write_icc_igrpen1_el1(pcpu_ctx[core_pos].icc_igrpen1);
    isb();
}

void gicv3_save_sgi_ppi_context(void)
{
    unsigned int i, core_pos;
    unsigned int my_core_pos = platform_get_core_pos(read_mpidr_el1());

    /* Save the context for all the suspended cores */
    for (core_pos = 0; core_pos < PLATFORM_CPU_COUNT; core_pos++) {
        /*
         * Continue if the core pos is not the current core
         * and has not suspended
         */
        if ((core_pos != my_core_pos) &&
                (!pcpu_ctx[core_pos].is_suspended))
            continue;

        assert(rdist_pcpu_base[core_pos]);

        pcpu_ctx[core_pos].gicr_isenabler =
                    gicr_read_isenabler0(rdist_pcpu_base[core_pos]);

        /* Read the ipriority registers, 4 at a time */
        for (i = 0; i < (NUM_PCPU_INTR >> IPRIORITYR_SHIFT); i++)
            pcpu_ctx[core_pos].gicr_ipriorityr[i] =
                gicr_read_ipriorityr(rdist_pcpu_base[core_pos],
                        i << IPRIORITYR_SHIFT);

        pcpu_ctx[core_pos].gicr_icfgr =
                gicr_read_icfgr1(rdist_pcpu_base[core_pos]);
    }
}

void gicv3_restore_sgi_ppi_context(void)
{
    unsigned int i, core_pos;
    unsigned int my_core_pos = platform_get_core_pos(read_mpidr_el1());

    /* Restore the context for all the suspended cores */
    for (core_pos = 0; core_pos < PLATFORM_CPU_COUNT; core_pos++) {
        /*
         * Continue if the core pos is not the current core
         * and has not suspended
         */
        if ((core_pos != my_core_pos) &&
                (!pcpu_ctx[core_pos].is_suspended))
            continue;

        assert(rdist_pcpu_base[core_pos]);

        /* Read the ipriority registers, 4 at a time */
        for (i = 0; i < (NUM_PCPU_INTR >> IPRIORITYR_SHIFT); i++)
            gicr_write_ipriorityr(rdist_pcpu_base[core_pos],
                    i << IPRIORITYR_SHIFT,
                    pcpu_ctx[core_pos].gicr_ipriorityr[i]);

        gicr_write_icfgr1(rdist_pcpu_base[core_pos],
                pcpu_ctx[core_pos].gicr_icfgr);
        gicr_write_isenabler0(rdist_pcpu_base[core_pos],
                pcpu_ctx[core_pos].gicr_isenabler);
    }
}

unsigned int gicv3_get_ipriorityr(unsigned int interrupt_id)
{
    unsigned int core_pos;

    assert(gicd_base_addr);
    assert(IS_VALID_INTR_ID(interrupt_id));

    if (interrupt_id < MIN_SPI_ID) {
        core_pos = platform_get_core_pos(read_mpidr_el1());
        assert(rdist_pcpu_base[core_pos]);
        return pal_mmio_read8(rdist_pcpu_base[core_pos] + GICR_IPRIORITYR
                + interrupt_id);
    } else {
        return pal_mmio_read8(gicd_base_addr +
            GICD_IPRIORITYR + interrupt_id);
    }
}

void gicv3_set_ipriorityr(unsigned int interrupt_id,
                unsigned int priority)
{
    unsigned int core_pos;

    assert(gicd_base_addr);
    assert(IS_VALID_INTR_ID(interrupt_id));

    if (interrupt_id < MIN_SPI_ID) {
        core_pos = platform_get_core_pos(read_mpidr_el1());
        assert(rdist_pcpu_base[core_pos]);
        pal_mmio_write8(rdist_pcpu_base[core_pos] + GICR_IPRIORITYR
                + interrupt_id, priority & GIC_PRI_MASK);
    } else {
        pal_mmio_write8(gicd_base_addr + GICD_IPRIORITYR + interrupt_id,
                    priority & GIC_PRI_MASK);
    }
}

void gicv3_send_sgi(unsigned int sgi_id, unsigned int core_pos)
{
    unsigned long long aff0, aff1, aff2;
    unsigned long long sgir, target_list;

    assert(IS_SGI(sgi_id));
    assert(core_pos < PLATFORM_CPU_COUNT);

    assert(mpidr_list[core_pos] != UINT64_MAX);

    /* Extract the affinity information */
    aff0 = MPIDR_AFF_ID(mpidr_list[core_pos], 0);
    aff1 = MPIDR_AFF_ID(mpidr_list[core_pos], 1);
    aff2 = MPIDR_AFF_ID(mpidr_list[core_pos], 2);
#ifdef __aarch64__
    unsigned long long aff3;

    aff3 = MPIDR_AFF_ID(mpidr_list[core_pos], 3);
#endif

    /* Construct the SGI target list using Affinity 0 */
    assert(aff0 < SGI_TARGET_MAX_AFF0);
    target_list = 1ULL << aff0;

    /* Construct the SGI target affinity */
    sgir =
#ifdef __aarch64__
        ((aff3 & SGI1R_AFF_MASK) << SGI1R_AFF3_SHIFT) |
#endif
        ((aff2 & SGI1R_AFF_MASK) << SGI1R_AFF2_SHIFT) |
        ((aff1 & SGI1R_AFF_MASK) << SGI1R_AFF1_SHIFT) |
        ((target_list & SGI1R_TARGET_LIST_MASK)
                << SGI1R_TARGET_LIST_SHIFT);

    /* Combine SGI target affinity with the SGI ID */
    sgir |= ((sgi_id & SGI1R_INTID_MASK) << SGI1R_INTID_SHIFT);
#ifdef __aarch64__
    write_icc_sgi1r(sgir);
#else
    write64_icc_sgi1r(sgir);
#endif
    isb();
}

void gicv3_set_intr_route(unsigned int interrupt_id,
        unsigned int core_pos)
{
    unsigned long long route_affinity;

    assert(gicd_base_addr);
    assert(core_pos < PLATFORM_CPU_COUNT);
    assert(mpidr_list[core_pos] != UINT64_MAX);

    /* Routing information can be set only for SPIs */
    assert(IS_SPI(interrupt_id));
    route_affinity = mpidr_list[core_pos];

    gicd_write_irouter(gicd_base_addr, interrupt_id, route_affinity);
}

unsigned int gicv3_get_isenabler(unsigned int interrupt_id)
{
    unsigned int core_pos;

    assert(gicd_base_addr);
    assert(IS_VALID_INTR_ID(interrupt_id));

    if (interrupt_id < MIN_SPI_ID) {
        core_pos = platform_get_core_pos(read_mpidr_el1());
        assert(rdist_pcpu_base[core_pos]);
        return gicr_get_isenabler0(rdist_pcpu_base[core_pos], interrupt_id);
    } else
        return gicd_get_isenabler(gicd_base_addr, interrupt_id);
}

void gicv3_set_isenabler(unsigned int interrupt_id)
{
    unsigned int core_pos;

    assert(gicd_base_addr);
    assert(IS_VALID_INTR_ID(interrupt_id));

    if (interrupt_id < MIN_SPI_ID) {
        core_pos = platform_get_core_pos(read_mpidr_el1());
        assert(rdist_pcpu_base[core_pos]);
        gicr_set_isenabler0(rdist_pcpu_base[core_pos], interrupt_id);
    } else
        gicd_set_isenabler(gicd_base_addr, interrupt_id);
}

void gicv3_set_icenabler(unsigned int interrupt_id)
{
    unsigned int core_pos;

    assert(gicd_base_addr);
    assert(IS_VALID_INTR_ID(interrupt_id));

    if (interrupt_id < MIN_SPI_ID) {
        core_pos = platform_get_core_pos(read_mpidr_el1());
        assert(rdist_pcpu_base[core_pos]);
        gicr_set_icenabler0(rdist_pcpu_base[core_pos], interrupt_id);
    } else
        gicd_set_icenabler(gicd_base_addr, interrupt_id);
}

unsigned int gicv3_get_ispendr(unsigned int interrupt_id)
{
    unsigned int ispendr;
    unsigned int bit_pos, core_pos;

    assert(gicd_base_addr);
    assert(IS_VALID_INTR_ID(interrupt_id));

    if (interrupt_id < MIN_SPI_ID) {
        core_pos = platform_get_core_pos(read_mpidr_el1());
        assert(rdist_pcpu_base[core_pos]);
        ispendr = gicr_read_ispendr0(rdist_pcpu_base[core_pos]);
    } else
        ispendr = gicd_read_ispendr(gicd_base_addr, interrupt_id);

    bit_pos = interrupt_id % (1 << ISPENDR_SHIFT);
    return !!(ispendr & (1U << bit_pos));
}

void gicv3_set_icpendr(unsigned int interrupt_id)
{
    unsigned int core_pos;

    assert(gicd_base_addr);
    assert(IS_SPI(interrupt_id) || IS_PPI(interrupt_id));

    if (interrupt_id < MIN_SPI_ID) {
        core_pos = platform_get_core_pos(read_mpidr_el1());
        assert(rdist_pcpu_base[core_pos]);
        gicr_set_icpendr0(rdist_pcpu_base[core_pos], interrupt_id);

    } else
        gicd_set_icpendr(gicd_base_addr, interrupt_id);
}

void gicv3_probe_redistif_addr(void)
{
    unsigned long long typer_val;
    uintptr_t rdistif_base;
    unsigned long long affinity;
    unsigned int core_pos = platform_get_core_pos(read_mpidr_el1());

    assert(gicr_base_addr);

    /*
     * Return if the re-distributor base address is already populated
     * for this core.
     */
    if (rdist_pcpu_base[core_pos])
        return;

    /* Iterate over the GICR frames and find the matching frame*/
    rdistif_base = gicr_base_addr;
    affinity = gic_typer_affinity_from_mpidr(read_mpidr_el1() & MPIDR_AFFINITY_MASK);
    do {
        typer_val = gicr_read_typer(rdistif_base);
        if (affinity == ((typer_val >> TYPER_AFF_VAL_SHIFT) & TYPER_AFF_VAL_MASK)) {
            rdist_pcpu_base[core_pos] = rdistif_base;
            mpidr_list[core_pos] = read_mpidr_el1() & MPIDR_AFFINITY_MASK;
            return;
        }
        rdistif_base += (1 << GICR_PCPUBASE_SHIFT);
    } while (!(typer_val & TYPER_LAST_BIT));

    pal_printf("Re-distributor address not found for core %d\n", (uint64_t)core_pos, 0);
    //panic();
}

void gicv3_setup_distif(void)
{
    unsigned int gicd_ctlr;

    assert(gicd_base_addr);

    /* Check for system register support */
#ifdef __aarch64__
    assert(read_id_aa64pfr0_el1() &
            (ID_AA64PFR0_GIC_MASK << ID_AA64PFR0_GIC_SHIFT));
#else
    assert(read_id_pfr1() & (ID_PFR1_GIC_MASK << ID_PFR1_GIC_SHIFT));
#endif

    /* Assert that system register access is enabled */
    assert(is_sre_enabled());

    /* Enable the forwarding of interrupts to CPU interface */
    gicd_ctlr = gicd_read_ctlr(gicd_base_addr);

    /* Assert ARE_NS bit in GICD */
    assert(gicd_ctlr & (GICD_CTLR_ARE_NS_MASK << GICD_CTLR_ARE_NS_SHIFT));

    gicd_ctlr |= GICD_CTLR_ENABLE_GRP1A;
    gicd_write_ctlr(gicd_base_addr, gicd_ctlr);
}

void gicd_write_igroupr(uintptr_t base, unsigned int id, unsigned int val)
{
    GICD_WRITE(IGROUP, base, id, val);
}

void gicd_set_igrpmodr(uintptr_t base, unsigned int id)
{
    GICD_SET_BIT(IGRPMOD, base, id);
}

void gicd_clr_igrpmodr(uintptr_t base, unsigned int id)
{
    GICD_CLR_BIT(IGRPMOD, base, id);
}

void gicd_set_igroupr(uintptr_t base, unsigned int id)
{
    GICD_SET_BIT(IGROUP, base, id);
}

void gicd_clr_igroupr(uintptr_t base, unsigned int id)
{
    GICD_CLR_BIT(IGROUP, base, id);
}

/*
 * Accessors to set the bits corresponding to interrupt ID
 * in GIC Distributor ICFGR and ICFGRE.
 */
void gicd_set_icfgr(uintptr_t base, unsigned int id, unsigned int cfg)
{
    /* Interrupt configuration is a 2-bit field */
    unsigned int bit_shift = BIT_NUM(ICFG, id) << 1U;

    /* Clear the field, and insert required configuration */
    mmio_clrsetbits_32(base + GICD_OFFSET(ICFG, id),
                (uint32_t)GIC_CFG_MASK << bit_shift,
                (cfg & GIC_CFG_MASK) << bit_shift);
}

/*******************************************************************************
 * Helper function to configure properties of secure SPIs
 ******************************************************************************/
unsigned int gicv3_secure_spis_config_props(unsigned int intr_num)
{
    unsigned long long gic_affinity_val;
    unsigned int ctlr_enable = 0U;
    unsigned int intr_grp = 0;
    uint64_t gicd_base = GICD_BASE;
        /* Configure this interrupt as a secure interrupt */
        gicd_clr_igroupr(gicd_base, intr_num);

        if (intr_grp == INTR_GROUP1S) {
            gicd_set_igrpmodr(gicd_base, intr_num);
            ctlr_enable |= CTLR_ENABLE_G1S_BIT;
        } else {
            gicd_clr_igrpmodr(gicd_base, intr_num);
            ctlr_enable |= CTLR_ENABLE_G0_BIT;
        }

        /* Set interrupt configuration */
        gicd_set_icfgr(gicd_base, intr_num, 0);

        /* Set the priority of this interrupt */
        gicd_set_ipriorityr(gicd_base, intr_num,
                    0);

        /* Target SPIs to the primary CPU */
        gic_affinity_val =
            gicd_irouter_val_from_mpidr(read_mpidr_el1(), 0U);
        gicd_write_irouter(gicd_base, intr_num,
                    gic_affinity_val);

        /* Enable this interrupt */
        gicd_set_isenabler(gicd_base, intr_num);

    return ctlr_enable;
}

/*******************************************************************************
 * Helper function to get the maximum SPI INTID + 1.
 ******************************************************************************/
unsigned int gicv3_get_spi_limit(uintptr_t gicd_base)
{
    unsigned int spi_limit;
    unsigned int typer_reg = gicd_read_typer(gicd_base);

    /* (maximum SPI INTID + 1) is equal to 32 * (GICD_TYPER.ITLinesNumber+1) */
    spi_limit = ((typer_reg & TYPER_IT_LINES_NO_MASK) + 1U) << 5;

    /* Filter out special INTIDs 1020-1023 */
    if (spi_limit > (MAX_SPI_ID + 1U)) {
        return MAX_SPI_ID + 1U;
    }

    return spi_limit;
}

/*******************************************************************************
 * Helper function to configure the default attributes of SPIs.
 ******************************************************************************/
void gicv3_spis_config_defaults(uintptr_t gicd_base)
{
    unsigned int i, num_ints;

    num_ints = gicv3_get_spi_limit(gicd_base);
    // INFO("Maximum SPI INTID supported: %u\n", num_ints - 1);

    /* Treat all SPIs as G1NS by default. We do 32 at a time. */
    for (i = MIN_SPI_ID; i < num_ints; i += (1U << IGROUPR_SHIFT)) {
        gicd_write_igroupr(gicd_base, i, ~0U);
    }

    /* Setup the default SPI priorities doing four at a time */
    for (i = MIN_SPI_ID; i < num_ints; i += (1U << IPRIORITYR_SHIFT)) {
        gicd_write_ipriorityr(gicd_base, i, GICD_IPRIORITYR_DEF_VAL);
    }

    /*
     * Treat all SPIs as level triggered by default, write 16 at a time
     */
    for (i = MIN_SPI_ID; i < num_ints; i += (1U << ICFGR_SHIFT)) {
        gicd_write_icfgr(gicd_base, i, 0U);
    }
}

/*******************************************************************************
 * This function initialises the GIC distributor interface based upon the data
 * provided by the test while enabling the driver.
 ******************************************************************************/
void gicv3_configure_secure_gic_intr_enable(unsigned int num)
{
    unsigned int bitmap;

    /*
     * Clear the "enable" bits for G0/G1S/G1NS interrupts before configuring
     * the ARE_S bit. The Distributor might generate a system error
     * otherwise.
     */
    gicd_clr_ctlr(gicd_base_addr,
              CTLR_ENABLE_G0_BIT |
              CTLR_ENABLE_G1S_BIT |
              CTLR_ENABLE_G1NS_BIT,
              RWP_TRUE);

    /* Set the ARE_S and ARE_NS bit now that interrupts have been disabled */
    gicd_set_ctlr(gicd_base_addr,
            CTLR_ARE_S_BIT | CTLR_ARE_NS_BIT, RWP_TRUE);

    /* Set the default attribute of all SPIs */
    gicv3_spis_config_defaults(gicd_base_addr);

    bitmap = gicv3_secure_spis_config_props(num);

    /* Enable the secure SPIs now that they have been configured */
    gicd_set_ctlr(gicd_base_addr, bitmap, RWP_TRUE);
}

void gicv3_init(uintptr_t gicr_base, uintptr_t gicd_base)
{
    assert(gicr_base);
    assert(gicd_base);

    gicr_base_addr = gicr_base;
    gicd_base_addr = gicd_base;
}

