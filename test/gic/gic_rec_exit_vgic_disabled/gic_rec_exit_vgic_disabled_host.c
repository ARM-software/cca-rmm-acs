/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"

void gic_rec_exit_vgic_disabled_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;
    uint64_t hcr;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    if (val_host_realm_setup(&realm, 1)) {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);

    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return;
    }

    hcr = gic_host_ich_read_hcr();
    if (VAL_EXTRACT_BITS(hcr, 0, 0) != 0U) {
        LOG(ERROR, "ICH_HCR_EL2.En was not cleared on REC exit, hcr=%lx\n", hcr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        return;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));
}
