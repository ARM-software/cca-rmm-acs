/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_SMC_H_
#define _VAL_SMC_H_

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
} val_smc_param_ts;

#define VAL_SMC_NOT_SUPPORTED    (-1)

val_smc_param_ts val_smc_call(uint64_t x0, uint64_t x1, uint64_t x2,
                         uint64_t x3, uint64_t x4, uint64_t x5,
                         uint64_t x6, uint64_t x7, uint64_t x8,
                         uint64_t x9, uint64_t x10);
void val_smc_call_asm(val_smc_param_ts *args);
void val_return_to_host_hvc_asm(void);
void val_realm_printf_msg_hvc_asm(void);
#endif /* _VAL_SMC_H_ */
