/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_pmu.h"

#define RETAINED_COUNTER  1U

void pmu_host_retained_host(void)
{
    val_host_realm_ts realm = {0};
    val_host_rec_exit_ts *rec_exit;
    uint64_t feature_reg, ret;
    uint64_t host_pmcr, host_counter, host_event_type, host_pmselr;
    uint64_t host_counter_enabled;

    val_host_rmi_features(0, &feature_reg);
    if (VAL_EXTRACT_BITS(feature_reg, 26, 26) == 0 ||
        VAL_EXTRACT_BITS(feature_reg, 27, 31) == 0 ||
        VAL_EXTRACT_BITS(read_pmcr_el0(), PMCR_EL0_N_SHIFT,
                         PMCR_EL0_N_SHIFT + 4) <= RETAINED_COUNTER)
    {
        LOG(TEST, "A Host-retained PMU counter is not available\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    host_pmcr = read_pmcr_el0();
    host_pmselr = read_pmselr_el0();
    host_counter = read_pmevcntrn_el0(RETAINED_COUNTER);
    host_event_type = read_pmevtypern_el0(RETAINED_COUNTER);
    host_counter_enabled =
        read_pmcntenset_el0() & PMCNTENSET_EL0_P_BIT(RETAINED_COUNTER);

    write_pmevtypern_el0(RETAINED_COUNTER,
                         PMU_EVT_INST_RETIRED & PMEVTYPER_EL0_EVTCOUNT_BITS);
    write_pmcntenset_el0(PMCNTENSET_EL0_P_BIT(RETAINED_COUNTER));
    write_pmcr_el0(read_pmcr_el0() | PMCR_EL0_E_BIT);
    isb();

    val_host_realm_params(&realm);
    realm.flags = REALM_FLAG_PMU_ENABLE;
    realm.pmu_num_ctrs = 1;
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
        LOG(ERROR, "REC entry failed, ret=%lx exit=%lx\n", ret, rec_exit->exit_reason);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto cleanup;
    }

    if ((read_pmcntenset_el0() & PMCNTENSET_EL0_P_BIT(RETAINED_COUNTER)) == 0)
    {
        LOG(ERROR, "Host retained-counter enable state was not restored\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto cleanup;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    write_pmcntenclr_el0(PMCNTENSET_EL0_P_BIT(RETAINED_COUNTER));
    write_pmevcntrn_el0(RETAINED_COUNTER, host_counter);
    write_pmevtypern_el0(RETAINED_COUNTER, host_event_type);

    if (host_counter_enabled)
        write_pmcntenset_el0(PMCNTENSET_EL0_P_BIT(RETAINED_COUNTER));

    write_pmcr_el0(host_pmcr);
    write_pmselr_el0(host_pmselr);
    isb();
}
