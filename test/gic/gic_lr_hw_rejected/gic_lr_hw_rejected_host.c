/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"
#include "val_irq.h"

#define GIC_ICH_HCR_EL2_EN       (1ULL << 0)
#define GIC_ICH_VMCR_VENG1       (1ULL << 1)
#define GIC_ICH_VMCR_VPMR_SHIFT  24
#define GIC_UNMASKED_VPMR        0xffU

void gic_lr_hw_rejected_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;
    uint32_t states[] = {
        GICV3_LR_STATE_PENDING,
        GICV3_LR_STATE_ACTIVE,
        GICV3_LR_STATE_PENDING_AND_ACTIVE,
    };
    uint32_t num_lrs;
    uint32_t lr_idx;
    uint32_t i;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    if (val_host_realm_setup(&realm, 1)) {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    num_lrs = gic_host_ich_num_lrs();

    for (lr_idx = 0U; lr_idx < num_lrs; lr_idx++) {
        for (i = 0U; i < (sizeof(states) / sizeof(states[0])); i++) {
            gic_host_ich_clear();
            gic_host_ich_write_vmcr(GIC_ICH_VMCR_VENG1 |
                                    ((uint64_t)GIC_UNMASKED_VPMR <<
                                     GIC_ICH_VMCR_VPMR_SHIFT));
            gic_host_ich_write_lr(lr_idx,
                                  ((uint64_t)states[i] << GICV3_LR_STATE) |
                                  (1ULL << GICV3_LR_HW) |
                                  (1ULL << GICV3_LR_GROUP) |
                                  ((uint64_t)SPI_vINTID << GICV3_LR_pINTID) |
                                  SPI_vINTID);
            gic_host_ich_write_hcr(GIC_ICH_HCR_EL2_EN);

            ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
            if (ret == RMI_SUCCESS) {
                LOG(ERROR, "REC entry accepted ICH_LR%u_EL2.HW == 1, state=%u\n",
                    lr_idx, states[i]);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
                gic_host_ich_clear();
                return;
            }

            if (RMI_STATUS(ret) != RMI_ERROR_REC) {
                LOG(ERROR, "Unexpected REC entry failure for ICH_LR%u_EL2.HW == 1, "
                           "ret=%lx state=%u\n", lr_idx, ret, states[i]);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
                gic_host_ich_clear();
                return;
            }
        }
    }

    gic_host_ich_clear();
    val_set_status(RESULT_PASS(VAL_SUCCESS));
}
