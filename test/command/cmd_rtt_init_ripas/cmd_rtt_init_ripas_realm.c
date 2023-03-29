/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_psci.h"
#include "val_realm_framework.h"

void cmd_rtt_init_ripas_realm(void)
{
    /* Execute PSCI sytem off which should change the realm(rd).state to SYSTEM_OFF */
    val_psci_system_off();
}


