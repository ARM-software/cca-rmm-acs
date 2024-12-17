/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <config.h>

#define assert_func PLAT_ASSERT_FUNC

extern void assert_func(const char *e, uint64_t line, const char *file);

#define assert(e)   ((e) ? (void)0 : assert_func(#e, __LINE__, __FILE__))
