/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _PAL_SMC_H_
#define _PAL_SMC_H_

#include "pal.h"

typedef struct {
    uint64_t x0;
    uint64_t x1;
    uint64_t x2;
    uint64_t x3;
    uint64_t x4;
    uint64_t x5;
    uint64_t x6;
    uint64_t x7;
} pal_smc_param_t;

pal_smc_param_t pal_smc_call(uint64_t x0, uint64_t x1, uint64_t x2,
                         uint64_t x3, uint64_t x4, uint64_t x5,
                         uint64_t x6, uint64_t x7);
void pal_smc_call_asm(pal_smc_param_t *args);
#endif /* _PAL_SMC_H_ */
