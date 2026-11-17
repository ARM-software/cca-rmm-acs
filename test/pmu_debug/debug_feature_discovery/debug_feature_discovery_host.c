/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"

void debug_feature_discovery_host(void)
{
    uint64_t dfr0;
    uint64_t dfr1;
    uint64_t feature_reg;
    uint64_t num_bps;
    uint64_t num_wps;
    uint64_t rmi_num_bps;
    uint64_t rmi_num_wps;

    dfr0 = read_id_aa64dfr0_el1();
    dfr1 = read_id_aa64dfr1_el1();

    /*
     * ID_AA64DFR0_EL1.BRPs and WRPs contain the implemented count
     * minus one. When either field is 0xf, ID_AA64DFR1_EL1 carries
     * the extended value. A zero extended value retains 0xf.
     */
    num_bps = VAL_EXTRACT_BITS(dfr0, 12, 15);
    if (num_bps == 0xF) {
        num_bps = VAL_EXTRACT_BITS(dfr1, 8, 15);
        if (num_bps == 0)
            num_bps = 0xF;
    }

    num_wps = VAL_EXTRACT_BITS(dfr0, 20, 23);
    if (num_wps == 0xF) {
        num_wps = VAL_EXTRACT_BITS(dfr1, 16, 23);
        if (num_wps == 0)
            num_wps = 0xF;
    }

    val_host_rmi_features(0, &feature_reg);

    rmi_num_bps = VAL_EXTRACT_BITS(feature_reg, 14, 19);
    rmi_num_wps = VAL_EXTRACT_BITS(feature_reg, 20, 25);

    if (rmi_num_bps != num_bps) {
        LOG(ERROR, "NUM_BPS mismatch: RMI=%lu, ID registers=%lu\n",
                    rmi_num_bps, num_bps);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    if (rmi_num_wps != num_wps) {
        LOG(ERROR, "NUM_WPS mismatch: RMI=%lu, ID registers=%lu\n",
                    rmi_num_wps, num_wps);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));
}
