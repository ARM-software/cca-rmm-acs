/*
 * Copyright (c) 2023, 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_realm_framework.h"
#include "val_psci.h"

#define CPU_SUSPEND_POWER_STATE    0x12345678UL
#define CPU_SUSPEND_ENTRY_POINT    0x12345000UL
#define CPU_SUSPEND_CONTEXT_ID     0x123456789ABCDEF0UL

void cmd_cpu_suspend_realm(void)
{
    /* Execute PSCI_CPU_SUSPEND which causes REC exit due to PSCI
     * RMM treats all target power states as suspend and ignores the input parameters */
    val_psci_cpu_suspend(CPU_SUSPEND_POWER_STATE,
                         CPU_SUSPEND_ENTRY_POINT,
                         CPU_SUSPEND_CONTEXT_ID);
}
