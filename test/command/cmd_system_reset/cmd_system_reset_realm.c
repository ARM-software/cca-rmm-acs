/*
 * Copyright (c) 2023, 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_realm_framework.h"
#include "val_psci.h"


void cmd_system_reset_realm(void)
{
    /* Execute PSCI_SYSTEM_RESET which causes REC exit due to PSCI */
    val_psci_system_reset();

    LOG(ERROR, "Unexpected REC enter\n");
    val_realm_return_to_host();
}


