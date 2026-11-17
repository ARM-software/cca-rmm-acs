/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"

#define GIC_ICH_HCR_EL2_EN       (1ULL << 0)
#define GIC_ICH_HCR_EL2_TDIR     (1ULL << 14)
#define GIC_ICH_VMCR_VENG1       (1ULL << 1)
#define GIC_ICH_VMCR_VPMR_SHIFT  24
#define GIC_UNMASKED_VPMR        0xffU

void gic_trapped_icc_sysreg_access_host(void)
{
    val_host_realm_ts realm;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    if (val_host_realm_setup(&realm, 1)) {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    gic_host_ich_program(GIC_ICH_HCR_EL2_EN | GIC_ICH_HCR_EL2_TDIR,
                         GIC_ICH_VMCR_VENG1 |
                         ((uint64_t)GIC_UNMASKED_VPMR << GIC_ICH_VMCR_VPMR_SHIFT),
                         0U);

    if (gic_host_expect_sysreg_exit(&realm) != VAL_SUCCESS) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        gic_host_ich_clear();
        return;
    }

    gic_host_ich_clear();
    val_set_status(RESULT_PASS(VAL_SUCCESS));
}
