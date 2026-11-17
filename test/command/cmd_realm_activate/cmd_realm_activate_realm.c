/*
 * Copyright (c) 2023, 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_psci.h"
#include "val_realm_framework.h"
void cmd_realm_activate_realm(void)
{
    /* Execute PSCI system off which should change the realm(rd).state to SYSTEM_OFF */
    val_psci_system_off();

    LOG(ERROR, "PSCI_SYSTEM_OFF unexpectedly returned\n");
    val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
    val_realm_return_to_host();
}
