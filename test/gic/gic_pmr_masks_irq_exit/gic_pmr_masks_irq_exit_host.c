/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"
#include "val_host_realm.h"
#include "val_irq.h"

#define GIC_ICH_HCR_EL2_EN       (1ULL << 0)
#define GIC_ICH_VMCR_VENG1       (1ULL << 1)
#define GIC_ICH_VMCR_VPMR_SHIFT  24
#define GIC_MASKED_VPMR          0x00U
#define GIC_LOW_PRIORITY         0x80U

void gic_pmr_masks_irq_exit_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;
    val_host_rec_exit_ts *rec_exit;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    if (val_host_realm_setup(&realm, 1)) {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);

    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return;
    }

    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "Expected Host-call REC exit during IRQ handler setup, "
                   "exit_reason=%x esr=%lx\n",
            rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        return;
    }

    gic_host_ich_program(GIC_ICH_HCR_EL2_EN,
                         GIC_ICH_VMCR_VENG1 |
                         ((uint64_t)GIC_MASKED_VPMR << GIC_ICH_VMCR_VPMR_SHIFT),
                         gic_lr_prio(SPI_vINTID, GICV3_LR_STATE_PENDING,
                                     GIC_LOW_PRIORITY));

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);

    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        gic_host_ich_clear();
        return;
    }

    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "Expected Host-call REC exit with masked interrupt, "
                   "exit_reason=%x esr=%lx\n",
            rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        gic_host_ich_clear();
        return;
    }

    gic_host_ich_clear();

    if (IS_TEST_START(val_get_status()))
        val_set_status(RESULT_PASS(VAL_SUCCESS));
}
