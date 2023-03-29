 /*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __EXCEPTION_COMMON_REALM__
#define __EXCEPTION_COMMON_REALM__

#include "test_database.h"
#include "val_realm_framework.h"
#include "val_mp_supp.h"
#include "val_realm_rsi.h"
#include "val_exceptions.h"

#define EXCEPTION_TEST_MAX_GPRS 31
#define EXCEPTION_TEST_HOSTCALL_GPRS_CHECK_COUNT 7
#define EXCEPTION_TEST_PSCI_GPRS_CHECK_COUNT 7

#define MPIDR_AFFLVL_MASK    ULL(0xff)
#define MPIDR_AFF3_SHIFT    U(32)

#define MPIDR_AFFLVL3_MASK    ((unsigned long long)MPIDR_AFFLVL_MASK << MPIDR_AFF3_SHIFT)
#define EXCEPTION_AFFINITY_FROM_MPIDR(mpidr)    \
    (((mpidr) & (~MPIDR_AFFLVL3_MASK)) | (((mpidr) & MPIDR_AFFLVL3_MASK) >> 8))

/* abort type enum */
typedef enum _abort_type {
    EXCEPTION_ABORT_TYPE_NONE,
    EXCEPTION_ABORT_TYPE_HVC,
    EXCEPTION_ABORT_TYPE_IA,
    EXCEPTION_ABORT_TYPE_DA
} abort_type_te;

/* common data structure to hold the excetion related values
 * this will grow based on the scenario and need
 */
typedef struct _sea_parms_ts {
    uint64_t esr_el1;
    uint64_t far_el1;
    uint64_t next_pc;
    uint64_t ec;
    uint64_t is_exception_handled;
    uint64_t status;
    abort_type_te abort_type;
} sea_parms_ts;

bool synchronized_exception_handler(void);

#endif /*#ifndef __EXCEPTION_COMMON_REALM__*/
