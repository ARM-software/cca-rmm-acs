/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/* This file is derived from xlat_table_v2 library in TF-A project */

#ifndef XLAT_TABLES_ARCH_H
#define XLAT_TABLES_ARCH_H

#ifdef __aarch64__
#include "xlat_tables_aarch64.h"
#else
#include "xlat_tables_aarch32.h"
#endif

/*
 * Evaluates to 1 if the given physical address space size is a power of 2,
 * or 0 if it's not.
 */
#define CHECK_PHY_ADDR_SPACE_SIZE(size)				\
	(IS_POWER_OF_TWO(size))

/*
 * Compute the number of entries required at the initial lookup level to address
 * the whole virtual address space.
 */
#define GET_NUM_BASE_LEVEL_ENTRIES(addr_space_size)			\
	((addr_space_size) >>						\
		XLAT_ADDR_SHIFT(GET_XLAT_TABLE_LEVEL_BASE(addr_space_size)))

#endif /* XLAT_TABLES_ARCH_H */
