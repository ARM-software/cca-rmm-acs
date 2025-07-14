/*
 * Copyright (c) 2023, 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_framework.h"

#define CONTEXT_ID 0x5555

void cmd_psci_complete_realm(void)
{
    /* Below code is executed for REC[0] only */
    LOG(TEST, "In realm, Excecution PSCI CPU ON for REC_1\n");

    /* Power on REC[1] execution */
    val_psci_cpu_on(REC_NUM(1), val_realm_get_secondary_cpu_entry(), CONTEXT_ID);
}
