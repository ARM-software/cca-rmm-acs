/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */


#include "val_hvc.h"

/* HVC call */
val_hvc_param_ts val_hvc_call(uint64_t x0, uint64_t x1, uint64_t x2,
                                uint64_t x3, uint64_t x4, uint64_t x5,
                                uint64_t x6, uint64_t x7, uint64_t x8,
                                uint64_t x9, uint64_t x10)
{
    val_hvc_param_ts args;

    args.x0 = x0;
    args.x1 = x1;
    args.x2 = x2;
    args.x3 = x3;
    args.x4 = x4;
    args.x5 = x5;
    args.x6 = x6;
    args.x7 = x7;
    args.x8 = x8;
    args.x9 = x9;
    args.x10 = x10;
    val_hvc_call_asm(&args);
    return args;
}
