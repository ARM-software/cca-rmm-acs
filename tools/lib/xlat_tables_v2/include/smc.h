/*
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef SMC_H
#define SMC_H

/* SMCCC return codes */
#define SMC_SUCCESS     (unsigned long)(0)
#define SMC_NOT_SUPPORTED   (unsigned long)(-1)
#define SMC_NOT_REQUIRED    (unsigned long)(-2)
#define SMC_INVALID_PARAMETER   (unsigned long)(-3)

#define SMC_UNKNOWN     (unsigned long)(-1)

#ifndef __ASSEMBLER__
unsigned long monitor_call(unsigned long id,
            unsigned long arg0,
            unsigned long arg1,
            unsigned long arg2,
            unsigned long arg3,
            unsigned long arg4,
            unsigned long arg5);

/* Result registers X0-X4 */
#define SMC_RESULT_REGS     5U

struct smc_result {
    unsigned long x[SMC_RESULT_REGS];
};

void monitor_call_with_res(unsigned long id,
               unsigned long arg0,
               unsigned long arg1,
               unsigned long arg2,
               unsigned long arg3,
               unsigned long arg4,
               unsigned long arg5,
               struct smc_result *res);

#endif /* __ASSEMBLER__ */

#endif /* SMC_H */
