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

#endif /*#ifndef __EXCEPTION_COMMON_REALM__*/
