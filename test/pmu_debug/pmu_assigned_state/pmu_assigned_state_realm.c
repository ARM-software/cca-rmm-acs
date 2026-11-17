/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "test_database.h"
#include "val_pmu.h"
#include "val_realm_framework.h"

#define REALM_COUNTER_VALUE  0x5a5a1234UL
#define REALM_EVENT_TYPE     (PMU_EVT_SW_INCR & PMEVTYPER_EL0_EVTCOUNT_BITS)

void pmu_assigned_state_realm(void)
{
    uint64_t event_type;

    pmu_reset();
    write_pmevcntrn_el0(0, REALM_COUNTER_VALUE);
    write_pmevtypern_el0(0, REALM_EVENT_TYPE);
    write_pmcntenset_el0(PMCNTENSET_EL0_P_BIT(0));
    isb();

    /* RLHRYJ: exit after installing distinctive assigned-counter state. */
    val_realm_return_to_host();

    event_type = read_pmevtypern_el0(0) & PMEVTYPER_EL0_EVTCOUNT_BITS;
    if (read_pmevcntrn_el0(0) != REALM_COUNTER_VALUE)
    {
        LOG(ERROR, "RDNNQQ: assigned counter value was not restored\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }
    if (event_type != REALM_EVENT_TYPE)
    {
        LOG(ERROR, "RDNNQQ: assigned event type was not restored: %lx\n", event_type);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }
    if ((read_pmcntenset_el0() & PMCNTENSET_EL0_P_BIT(0)) == 0)
    {
        LOG(ERROR, "RDNNQQ: assigned counter enable was not restored\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }
    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    pmu_reset();
    val_realm_return_to_host();
}
