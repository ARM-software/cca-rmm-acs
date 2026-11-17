/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "test_database.h"
#include "val_pmu.h"
#include "val_realm_framework.h"

#define RETAINED_COUNTER  1U

void pmu_host_retained_realm(void)
{
    if (read_pmcntenset_el0() & PMCNTENSET_EL0_P_BIT(RETAINED_COUNTER))
    {
        LOG(ERROR, "RXZBJD: Host-retained counter is enabled in Realm\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    val_realm_return_to_host();
}
