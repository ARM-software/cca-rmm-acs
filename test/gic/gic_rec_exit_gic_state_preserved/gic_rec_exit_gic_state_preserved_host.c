/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"
#include "val_irq.h"

void gic_rec_exit_gic_state_preserved_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;
    uint64_t lr0;

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

    gic_host_ich_program(1U,
                           (1ULL << 1) | (0xffULL << 24),
                           gic_lr(SPI_vINTID, GICV3_LR_STATE_PENDING));

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);

    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        gic_host_ich_clear();
        return;
    }

    lr0 = gic_host_ich_read_lr0();
    if (VAL_EXTRACT_BITS(lr0, 62, 63) != GICV3_LR_STATE_ACTIVE) {
        LOG(ERROR, "Expected active LR state on REC exit, received=%x\n",
            VAL_EXTRACT_BITS(lr0, 62, 63));
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        gic_host_ich_clear();
        return;
    }

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);

    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        gic_host_ich_clear();
        return;
    }

    lr0 = gic_host_ich_read_lr0();
    if (VAL_EXTRACT_BITS(lr0, 62, 63) != GICV3_LR_STATE_INACTIVE) {
        LOG(ERROR, "Expected inactive LR state after EOIR, received=%x\n",
            VAL_EXTRACT_BITS(lr0, 62, 63));
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        gic_host_ich_clear();
        return;
    }

    gic_host_ich_clear();

    val_set_status(RESULT_PASS(VAL_SUCCESS));
}
