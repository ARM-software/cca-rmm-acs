/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_HVC_H_
#define _VAL_HVC_H_

#include "val.h"
#include "val_rmm.h"

typedef struct {
    uint64_t x0;
    uint64_t x1;
    uint64_t x2;
    uint64_t x3;
    uint64_t x4;
    uint64_t x5;
    uint64_t x6;
    uint64_t x7;
    uint64_t x8;
    uint64_t x9;
    uint64_t x10;
} val_hvc_param_ts;

#define VAL_HVC_NOT_SUPPORTED    (-1)

val_hvc_param_ts val_hvc_call(uint64_t x0, uint64_t x1, uint64_t x2,
                         uint64_t x3, uint64_t x4, uint64_t x5,
                         uint64_t x6, uint64_t x7, uint64_t x8,
                         uint64_t x9, uint64_t x10);
void val_hvc_call_asm(val_hvc_param_ts *args);
#endif /* _VAL_HVC_H_ */
