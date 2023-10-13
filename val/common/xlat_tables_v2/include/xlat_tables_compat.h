/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/* This file is derived from xlat_table_v2 library in TF-A project */

#ifndef XLAT_TABLES_COMPAT_H
#define XLAT_TABLES_COMPAT_H

#if XLAT_TABLES_LIB_V2
#include <lib/xlat_tables/xlat_tables_v2.h>
#else
#include <lib/xlat_tables/xlat_tables.h>
#endif

#endif /* XLAT_TABLES_COMPAT_H */
