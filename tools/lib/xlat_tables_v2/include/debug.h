/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <config.h>

typedef enum {
    INFO    = 1,
    DBG     = 2,
    TEST    = 3,
    WARN    = 4,
    ERROR   = 5,
    ALWAYS  = 9
} print_verbosity_t;

#define print_func PLAT_PRINT_FUNC

extern void print_func(const char *x, uint64_t y, uint64_t z);

#define LOG(print_verbosity, x, y, z)               \
   do {                                             \
    if (print_verbosity >= VERBOSITY)               \
        print_func(x, y, z);                        \
    if (print_verbosity == ERROR)                   \
    {                                               \
        print_func("\t(Check failed at:", 0, 0);    \
        print_func(__FILE__, 0, 0);                 \
        print_func(" ,line:%d)\n", __LINE__, 0);    \
    }                                               \
   } while (0);




