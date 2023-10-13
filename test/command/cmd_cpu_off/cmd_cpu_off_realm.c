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


void cmd_cpu_off_realm(void)
{
    /* Execute PSCI_CPU_OFF which causes REC exit due to PSCI */
    val_psci_cpu_off();
}


