/*
 * Copyright (c) 2023, 2025-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_realm_framework.h"
#include "val_psci.h"

#define PSCI_CPU_FREEZE       0xC400000B

static const uint64_t supported_psci_fids[] = {
    PSCI_VERSION,
    PSCI_CPU_SUSPEND_AARCH64,
    PSCI_CPU_OFF,
    PSCI_CPU_ON_AARCH64,
    PSCI_AFFINITY_INFO_AARCH64,
    PSCI_SYSTEM_OFF,
    PSCI_SYSTEM_RESET,
    PSCI_FEATURES
};

void cmd_psci_features_realm(void)
{
    uint64_t ret, i;

    for (i = 0; i < (sizeof(supported_psci_fids) /
                     sizeof(supported_psci_fids[0])); i++) {
        /* Execute PSCI_FEATURES for each supported PSCI function. */
        ret = val_psci_features(supported_psci_fids[i]);

        if (ret != PSCI_E_SUCCESS) {
            LOG(ERROR, "PSCI_FEATURES returned 0x%lx for supported FID 0x%lx\n",
                ret, supported_psci_fids[i]);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto exit;
        }
    }

    /* Query with an unsupported PSCI function */
    ret = val_psci_features(PSCI_CPU_FREEZE);

    /* Command should return PSCI_NOT_SUPPORTED */
    if (ret != PSCI_E_NOT_SUPPORTED) {
        LOG(ERROR, "PSCI_FEATURES returned 0x%lx for unsupported FID 0x%x\n",
            ret, PSCI_CPU_FREEZE);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    val_realm_return_to_host();

}
