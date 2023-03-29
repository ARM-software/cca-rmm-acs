/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __MM_COMMON_REALM__
#define __MM_COMMON_REALM__

#include "test_database.h"

/* common data structure to hold the excetion related values */
typedef struct _sea_params_ts {
    uint64_t far;
    uint64_t ec;
    uint64_t handler_abort;
    uint64_t abort_type;
} sea_params_ts;

bool synchronous_exception_handler(void);

#endif /* #ifndef __MMU_COMMON_REALM__ */
