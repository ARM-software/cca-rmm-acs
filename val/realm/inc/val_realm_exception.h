/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __VAL_REALM_EXCEPTION__
#define __VAL_REALM_EXCEPTION__

#include "test_database.h"

/* abort type enum */
typedef enum _abort_type {
    EXCEPTION_ABORT_TYPE_NONE,
    EXCEPTION_ABORT_TYPE_HVC,
    EXCEPTION_ABORT_TYPE_IA,
    EXCEPTION_ABORT_TYPE_DA
} abort_type_te;

/* common data structure to hold the excetion related values */
typedef struct _sea_params_ts {
    uint64_t far;
    uint64_t ec;
    uint64_t handler_abort;
    uint64_t abort_type;
} sea_params_ts;

bool synchronous_exception_handler(void);

#endif /* #ifndef __VAL_REALM_EXCEPTION__ */

