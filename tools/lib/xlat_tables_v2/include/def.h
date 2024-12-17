/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __DEF_H_
#define __DEF_H_

#include "arch.h"

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

#endif /* _VAL_DEF_H_ */
