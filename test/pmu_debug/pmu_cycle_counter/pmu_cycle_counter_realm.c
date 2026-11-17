/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "test_database.h"
#include "val_pmu.h"
#include "val_realm_framework.h"

#define REALM_CYCLE_VALUE  0x31415926UL
#define HOST_CYCLE_VALUE   0x10000000UL

void pmu_cycle_counter_realm(void)
{
    uint64_t before, after;
    volatile uint64_t delay;

    /* Host disabled: this value must be saved to, and later restored from, REC. */
    write_pmccntr_el0(REALM_CYCLE_VALUE);
    val_realm_return_to_host();

    /* Host enabled: preserve Host state instead of restoring REC state. */
    before = read_pmccntr_el0();
    if (before < HOST_CYCLE_VALUE || before == REALM_CYCLE_VALUE)
    {
        LOG(ERROR, "Host cycle counter not preserved on Realm entry: 0x%lx\n",
                    before);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    for (delay = 0; delay < 0x10000; delay++)
        ;
    after = read_pmccntr_el0();
    if (after <= before)
    {
        LOG(ERROR, "Cycle counter did not advance in Realm\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    val_realm_return_to_host();

    /* Host disabled again: the REC value from the first exit is restored. */
    if (read_pmccntr_el0() != REALM_CYCLE_VALUE)
    {
        LOG(ERROR, "REC cycle counter was not restored: 0x%lx\n",
                    read_pmccntr_el0());
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    val_realm_return_to_host();
}
