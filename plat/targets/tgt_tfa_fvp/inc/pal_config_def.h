/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _PAL_CONFIG_H_
#define _PAL_CONFIG_H_

/* To enable WFI test */
#define TEST_WFI_TRAP


/* Total number of CPUs(PEs) in system. ACS requires minimum of 2 CPUs.
 * Example:
 * 2 clusters, [2:2] cores in [1st:2nd] cluster : PLATFORM_CPU_COUNT => 4
 * 2 clusters, [2:4] cores in [1st:2nd] cluster : PLATFORM_CPU_COUNT => 6
 * 2 clusters, [4:4] cores in [1st:2nd] cluster : PLATFORM_CPU_COUNT => 8
 * */
#define PLATFORM_CPU_COUNT 8

/* MPIDR_EL1.{Aff3, Aff2, Aff1, Aff0} value for each physical CPU
 *  Bits[40:63]: Must be zero
 *  Bits[32:39] Aff3: Match Aff3 of target core MPIDR
 *  Bits[24:31] Must be zero
 *  Bits[16:23] Aff2: Match Aff2 of target core MPIDR
 *  Bits[8:15] Aff1: Match Aff1 of target core MPIDR
 *  Bits[0:7] Aff0: Match Aff0 of target core MPIDR
 *  */
#define PLATFORM_PHY_MPIDR_CPU0 0x00000
#define PLATFORM_PHY_MPIDR_CPU1 0x00100
#define PLATFORM_PHY_MPIDR_CPU2 0x00200
#define PLATFORM_PHY_MPIDR_CPU3 0x00300
#define PLATFORM_PHY_MPIDR_CPU4 0x10000
#define PLATFORM_PHY_MPIDR_CPU5 0x10100
#define PLATFORM_PHY_MPIDR_CPU6 0x10200
#define PLATFORM_PHY_MPIDR_CPU7 0x10300

/*
 * Device Info in physical addresses.
 */

/* Non-secure UART - PL011_UART2_BASE */
#define PLATFORM_NS_UART_BASE    0x1c0b0000
#define PLATFORM_NS_UART_SIZE    0x10000

/* Non-volatile memory range assigned */
#define PLATFORM_NVM_BASE    (0x80000000+0x2800000)
#define PLATFORM_NVM_SIZE    0x10000

/* Base address of watchdog assigned */
#define PLATFORM_WDOG_BASE    0x1C0F0000 //(SP805)
#define PLATFORM_WDOG_SIZE    0x10000
#define PLATFORM_WDOG_LOAD_VALUE (0x3E7 * 40 * 1000) // 20sec
#define PLATFORM_WDOG_INTR 32

#define PLATFORM_NS_WD_BASE  0x2A440000
#define PLATFORM_NS_WD_SIZE  0x1000
#define PLATFORM_NS_WD_INTR  59

/* Base address of trusted watchdog (SP805) */
#define PLATFORM_SP805_TWDOG_BASE    0x2A490000
#define PLATFORM_TWDOG_SIZE          0x10000
#define PLATFORM_TWDOG_INTID         56
#define ARM_SP805_TWDG_CLK_HZ   32768

/* Interrupts used for GIC testing */
#define SPI_vINTID 59
#define PPI_vINTID 27
#define SGI_vINTID 12
/* PMU physical interrupt */
#define PMU_PPI     23UL
/* PMU virtual interrupt */
#define PMU_VIRQ    PMU_PPI

/* ACS Memory Usage Layout
 *
 * +--------------+      +-------------+
 * |              |      | Host Image  |
 * |    ACS       |      |    (1MB)    |
 * | Normal World | ==>  +-------------+
 * |    Image     |      | Realm Image |
 * | (2MB Size)   |      |    (1MB)    |
 * +--------------+      +-------------+
 * |  Memory Pool |      |Shared Region|
 * |    (50MB)    |      |    (1MB)    |
 * |              | ==>  +-------------+
 * |              |      |             |
 * |              |      |    Heap     |
 * |              |      |   Memory    |
 * |              |      |    (49MB)   |
 * +--------------+      +-------------+
 *
 * 2MB for Image loading and 50MB as Free NS Space.
 */

#define PLATFORM_NORMAL_WORLD_IMAGE_SIZE  0x200000
#define PLATFORM_HOST_IMAGE_SIZE          (PLATFORM_NORMAL_WORLD_IMAGE_SIZE / 2)
#define PLATFORM_REALM_IMAGE_SIZE         0xC0000 //768 kb
#define PLATFORM_MEMORY_POOL_SIZE         (50 * 0x100000)
#define PLATFORM_SHARED_REGION_SIZE       0x100000
#define PLATFORM_HEAP_REGION_SIZE         (PLATFORM_MEMORY_POOL_SIZE \
                                             - PLATFORM_SHARED_REGION_SIZE)

/*
 * Run-time address of the ACS Non-secure image. It has to match
 * the location where the DUT software loads the ACS NS Image.
 */
#define PLATFORM_NORMAL_WORLD_IMAGE_BASE     0x88000000
#define PLATFORM_HOST_IMAGE_BASE             PLATFORM_NORMAL_WORLD_IMAGE_BASE
#define PLATFORM_REALM_IMAGE_BASE            (PLATFORM_NORMAL_WORLD_IMAGE_BASE \
                                                 + PLATFORM_HOST_IMAGE_SIZE)
#define PLATFORM_MEMORY_POOL_BASE           (PLATFORM_NORMAL_WORLD_IMAGE_BASE \
                                                 + PLATFORM_NORMAL_WORLD_IMAGE_SIZE)

#define PLATFORM_SHARED_REGION_BASE         PLATFORM_MEMORY_POOL_BASE
#define PLATFORM_HEAP_REGION_BASE           (PLATFORM_SHARED_REGION_BASE + \
                                            PLATFORM_SHARED_REGION_SIZE)

/* ACS Secure code is designed such way that it can run at EL2S or EL1S
 *  This is configured through command line argument.
 * */
#ifndef PLATFORM_SECURE_IMAGE_EL
#define PLATFORM_SECURE_IMAGE_EL 0x2
#endif

/*
 * Run-time address of the ACS secure image. It has to match
 * the location where the DUT software loads the ACS S Image.
 */
#if (PLATFORM_SECURE_IMAGE_EL == 0x2)
#define PLATFORM_SECURE_IMAGE_BASE         0x6000000
#else
#define PLATFORM_SECURE_IMAGE_BASE         0x7000000
#endif

#define PLATFORM_SECURE_IMAGE_SIZE         0x100000

/*
 * Invalidate the instr cache and data cache for image regions.
 * This is to prevent re-use of stale data cache entries from
 * prior bootloader stages.
 */
#define PLATFORM_BOOT_CACHE_INVALIDATE

/* If platform set to SCR_EL3.GPF=0, then GPC faults are taken to NS EL2.
 * If SCR_EL3.GPF=1 and implementation supports injection of GPFs into NS EL2.
 */
#define PLATFORM_GPF_SUPPORT_NS_EL2 0x0

/*******************************************************************************
 * GIC-400 & interrupt handling related constants
 ******************************************************************************/
/* Base FVP compatible GIC memory map */
#define GICD_BASE       0x2f000000
#define GICR_BASE       0x2f100000
#define GICC_BASE       0x2c000000
#define GICD_SIZE       0x10000
#define GICR_SIZE       0x100000
#define GICC_SIZE       0x2000

/* Non-secure EL1 physical timer interrupt */
#define IRQ_PHY_TIMER_EL1           30
/* Non-secure EL1 virtual timer interrupt */
#define IRQ_VIRT_TIMER_EL1          27
/* Non-secure EL2 physical timer interrupt */
#define IRQ_PHY_TIMER_EL2           26

#define IPA_WIDTH_DEFAULT   32

#define PGT_IAS     IPA_WIDTH_DEFAULT
#define PAGT_OAS    IPA_WIDTH_DEFAULT

/* To enable WFE test */
//#define TEST_WFE_TRAP

/* XLAT related macros */
#define MAX_MMAP_REGIONS 1UL
#define MAX_XLAT_TABLES  1UL
#define PLAT_VIRT_ADDR_SPACE_SIZE (1ULL << PGT_IAS)
#define PLAT_PHY_ADDR_SPACE_SIZE  (1ULL << PAGT_OAS)

#endif /* _PAL_CONFIG_H_ */
