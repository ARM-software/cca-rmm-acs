/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_pmu.h"

#define HOST_CYCLE_VALUE     0x10000000UL
#define CORRUPT_CYCLE_VALUE  0x70000000UL

void pmu_cycle_counter_host(void)
{
    val_host_realm_ts realm = {0};
    val_host_rec_exit_ts *rec_exit;
    uint64_t feature_reg, ret, cycle_after;
    uint64_t host_pmcr, host_cycle_counter;
    uint64_t host_cycle_enabled;

    val_host_rmi_features(0, &feature_reg);
    if (VAL_EXTRACT_BITS(feature_reg, 26, 26) == 0)
    {
        LOG(TEST, "Realm PMU not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    host_pmcr = read_pmcr_el0();
    host_cycle_counter = read_pmccntr_el0();
    host_cycle_enabled =
        read_pmcntenset_el0() & PMCNTENSET_EL0_C_BIT;

    val_host_realm_params(&realm);
    realm.flags = REALM_FLAG_PMU_ENABLE;
    realm.pmu_num_ctrs = VAL_EXTRACT_BITS(feature_reg, 27, 31);

    /* First entry uses the Host-disabled cycle-counter path. */
    write_pmcntenclr_el0(PMCNTENSET_EL0_C_BIT);
    write_pmccntr_el0(CORRUPT_CYCLE_VALUE);
    isb();

    if (val_host_realm_setup(&realm, true))
    {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto cleanup;
    }

    rec_exit = &((val_host_rec_run_ts *)realm.run[0])->exit;
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret || val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "Disabled-cycle REC entry failed, ret=%lx exit=%lx\n",
                    ret, rec_exit->exit_reason);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto cleanup;
    }

    /* Enabled Host cycle counter must be preserved on entry and exit. */
    write_pmccntr_el0(HOST_CYCLE_VALUE);
    write_pmcntenset_el0(PMCNTENSET_EL0_C_BIT);
    write_pmcr_el0(read_pmcr_el0() | PMCR_EL0_E_BIT);
    isb();

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret || val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "Enabled-cycle REC entry failed, ret=%lx exit=%lx\n",
                    ret, rec_exit->exit_reason);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto cleanup;
    }

    cycle_after = read_pmccntr_el0();
    if (cycle_after <= HOST_CYCLE_VALUE)
    {
        LOG(ERROR, "Enabled Host cycle counter did not advance: 0x%lx\n",
                    cycle_after);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto cleanup;
    }

    /* Disabling again makes entry restore the value retained in the REC. */
    write_pmcntenclr_el0(PMCNTENSET_EL0_C_BIT);
    write_pmccntr_el0(CORRUPT_CYCLE_VALUE);
    isb();

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret || val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "Restored-cycle REC entry failed, ret=%lx exit=%lx\n",
                    ret, rec_exit->exit_reason);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto cleanup;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    write_pmcntenclr_el0(PMCNTENSET_EL0_C_BIT);
    write_pmccntr_el0(host_cycle_counter);

    if (host_cycle_enabled)
        write_pmcntenset_el0(PMCNTENSET_EL0_C_BIT);

    write_pmcr_el0(host_pmcr);
    isb();
}
