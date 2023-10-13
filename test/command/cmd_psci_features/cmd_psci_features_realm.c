/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_realm_framework.h"
#include "val_psci.h"

#define PSCI_CPU_FREEZE       0x8400000B

void cmd_psci_features_realm(void)
{
    uint64_t ret;

    /* Execure PSCI features for a supported PSCI function*/
    ret = val_psci_features(PSCI_AFFINITY_INFO_AARCH64);

    /* Command should return PSCI_SUCCESS */
    if (ret != PSCI_E_SUCCESS) {
        LOG(ERROR, "\t Unexpected output for PSCI_FEATURES \n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    /* Query with an unsupported PSCI function */
    ret = val_psci_features(PSCI_CPU_FREEZE);

    /* Command should return PSCI_NOT_SUPPORTED */
    if (ret != PSCI_E_NOT_SUPPORTED) {
        LOG(ERROR, "\t Unexpected output for PSCI_FEATURES \n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
    }

exit:
    val_realm_return_to_host();

}


