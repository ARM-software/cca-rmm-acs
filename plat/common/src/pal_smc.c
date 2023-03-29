/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */


#include "pal_smc.h"

pal_smc_param_t pal_smc_call(uint64_t x0, uint64_t x1, uint64_t x2,
                                uint64_t x3, uint64_t x4, uint64_t x5,
                                uint64_t x6, uint64_t x7)
{
    pal_smc_param_t args;

    args.x0 = x0;
    args.x1 = x1;
    args.x2 = x2;
    args.x3 = x3;
    args.x4 = x4;
    args.x5 = x5;
    args.x6 = x6;
    args.x7 = x7;
    pal_smc_call_asm(&args);
    return args;
}
