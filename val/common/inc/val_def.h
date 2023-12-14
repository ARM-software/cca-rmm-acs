/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_DEF_H_
#define _VAL_DEF_H_

#include "pal_config_def.h"
#include "val_arch.h"

/* ACS Version Info */
#define ACS_MAJOR_VERSION   1
#define ACS_MINOR_VERSION   0

#define IMAGE_SIZE        0x100000
#define INVALID_MPIDR     0xffffffff

#define STACK_SIZE          0x1000

#define VAL_TG0_4K  0x0
#define VAL_TG0_64K 0x1
#define VAL_TG0_16K 0x2
#define PAGE_SIZE_4K        0x1000
#define PAGE_SIZE_16K       (4 * 0x1000)
#define PAGE_SIZE_64K       (16 * 0x1000)
#define PAGE_BITS_4K        12
#define PAGE_BITS_16K       14
#define PAGE_BITS_64K       16

#define PLATFORM_PAGE_SIZE 0x1000

#if (PLATFORM_PAGE_SIZE == PAGE_SIZE_4K)
 #define PAGE_ALIGNMENT      PAGE_SIZE_4K
 #define PAGE_SIZE           PAGE_SIZE_4K
 #define TCR_TG0             VAL_TG0_4K
#elif (PLATFORM_PAGE_SIZE == PAGE_SIZE_16K)
 #define PAGE_ALIGNMENT      PAGE_SIZE_16K
 #define PAGE_SIZE           PAGE_SIZE_16K
 #define TCR_TG0             VAL_TG0_16K
#elif (PLATFORM_PAGE_SIZE == PAGE_SIZE_64K)
 #define PAGE_ALIGNMENT      PAGE_SIZE_64K
 #define PAGE_SIZE           PAGE_SIZE_64K
 #define TCR_TG0             VAL_TG0_64K
#else
 #error "Undefined value for PLATFORM_PAGE_SIZE"
#endif

/*******************************************************************************
 * Used to align variables on the biggest cache line size in the platform.
 * This is known only to the platform as it might have a combination of
 * integrated and external caches.
 ******************************************************************************/
#define CACHE_WRITEBACK_SHIFT     6
#define CACHE_WRITEBACK_GRANULE   (1 << CACHE_WRITEBACK_SHIFT)

#define VAL_SWITCH_TO_HOST  5
#define VAL_REALM_PRINT_MSG 6


/* ACS VA, IPA, PA mapping
 *
 * ACS Host stage1 follows flat mapping for VA to PA translation.
 * ACS Secure stage1 follows flat mapping for VA to PA translation.
 * ACS Realm stage1 follows flat mapping for VA to IPA translation.
 * ACS Realm Stage2 follows non-flat mapping for IPA to PA translation
 * and IPA layout for the same is as follows:
 *
 * Realm IPA Space Layout
 *
 * Protected Space - Base: 0x0
 * 0x000000 - 0x3fffff  Test use
 * 0x400000 - 0x4fffff  Realm Image regions
 * 0x500000 - 0x8fffff  Reserved
 * 0x900000 - (2^(ipa_width - 1) - 1) Test use
 *
 * Unprotected Space - Base: 2^(ipa_width - 1)
 * 0x000000 - 0x3fffff  Test use
 * 0x400000 - 0x4fffff  Reserved
 * 0x500000 - 0x5fffff  Shared NS region
 * 0x600000 - 0x8fffff  Reserved
 * 0x900000 - ((2^ipa_width) - 1) Test use
 *
 * */

//MSB=0
#define VAL_REALM_IMAGE_BASE_IPA 0x400000

/* Use this macro for test use IPA */
#define VAL_TEST_USE_IPA 0x0

//MSB is set at runtime based on ipa_width selected
#define VAL_NS_SHARED_REGION_IPA_OFFSET 0x600000

#endif /* _VAL_DEF_H_ */
