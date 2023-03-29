/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _PAL_H_
#define _PAL_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

typedef uintptr_t addr_t;

/* Status macro */
#define PAL_SUCCESS  0
#define PAL_ERROR    1

/* MPIDR macros */
#define PAL_MPIDR_AFFLVL_MASK    0xffull
#define PAL_MPIDR_AFFINITY_BITS  8
#define PAL_MPIDR_AFF0_SHIFT     0
#define PAL_MPIDR_AFF1_SHIFT     8u
#define PAL_MPIDR_AFF2_SHIFT     16u
#define PAL_MPIDR_AFF3_SHIFT     32u
#define PAL_MPIDR_AFFLVL0        0x0ull
#define PAL_MPIDR_AFFLVL1        0x1ull
#define PAL_MPIDR_AFFLVL2        0x2ull
#define PAL_MPIDR_AFFLVL3        0x3ull

#define PAL_MPIDR_AFFINITY_MASK ((PAL_MPIDR_AFFLVL_MASK << PAL_MPIDR_AFF3_SHIFT) | \
                 (PAL_MPIDR_AFFLVL_MASK << PAL_MPIDR_AFF2_SHIFT) | \
                 (PAL_MPIDR_AFFLVL_MASK << PAL_MPIDR_AFF1_SHIFT) | \
                 (PAL_MPIDR_AFFLVL_MASK << PAL_MPIDR_AFF0_SHIFT))


/*
 * An invalid MPID. This value can be used by functions that return an MPID to
 * indicate an error.
 */
#define PAL_INVALID_MPID        0xFFFFFFFFu

#endif /* _PAL_H_ */
