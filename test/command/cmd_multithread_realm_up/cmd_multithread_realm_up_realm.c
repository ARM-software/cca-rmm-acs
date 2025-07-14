/*
 * Copyright (c) 2023, 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_realm_framework.h"

#define CONTEXT_ID 0x5555

static uint8_t is_secondary_cpu_booted;

static void secondary_cpu(void)
{
    LOG(TEST, "In realm_create_realm REC[1], mpdir=%x\n", val_read_mpidr());
    is_secondary_cpu_booted = 0x1;
    val_psci_cpu_off();
}

void cmd_multithread_realm_up_realm(void)
{
    uint64_t ret;

    if (val_get_primary_mpidr() != val_read_mpidr())
        secondary_cpu();

    /* Below code is executed for REC[0] only */
    LOG(TEST, "In realm_create_realm REC[0], mpdir=%x\n", val_read_mpidr());

    /* Power on REC[1] execution */
    ret = val_psci_cpu_on(REC_NUM(1), val_realm_get_secondary_cpu_entry(), CONTEXT_ID);
    if (ret)
    {
        LOG(ERROR, "PSCI CPU ON failed with ret status : 0x%x \n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    if (is_secondary_cpu_booted == 0x1)
        val_set_status(RESULT_PASS(VAL_SUCCESS));
    else
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));

exit:
    val_realm_return_to_host();
}
