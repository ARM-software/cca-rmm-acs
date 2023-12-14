/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_psci.h"
#include "val_realm_framework.h"

#define CONTEXT_ID 0x5555

void cmd_rec_enter_realm(void)
{
    uint64_t ret;

    if (val_get_primary_mpidr() != val_read_mpidr())
    {
        /* Execute PSCI sytem off which should change the realm(rd).state to SYSTEM_OFF */
        val_psci_system_off();
    }

    /* Execute PSCI_CPU_ON */
    ret = val_psci_cpu_on(REC_NUM(1), val_realm_get_secondary_cpu_entry(), CONTEXT_ID);
    if (ret)
    {
        LOG(ERROR, "\n\tPSCI CPU ON failed with ret status : 0x%x \n", ret, 0);
        goto exit;
    }

exit:
    val_realm_return_to_host();
}


