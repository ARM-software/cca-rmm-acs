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


void cmd_cpu_suspend_realm(void)
{
    /* Execute PSCI_CPU_SUSPEND which causes REC exit due to PSCI
     * RMM treats all targed power states as suspend and ignores the input parameters */
    val_psci_cpu_suspend(0, 0, 0);
}


