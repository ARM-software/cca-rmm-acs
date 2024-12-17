/*
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"
#include "val_timer.h"

void exception_rec_exit_irq_realm(void)
{
    uint64_t timeout, time;
    val_realm_rsi_host_call_t *gv_realm_host_call;

    /* Return to host after initialization and ask for timeout */
    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    timeout = gv_realm_host_call->gprs[1];

    /* Below code is executed for REC[0] only */
    LOG(DBG, "\tIn Realm, REC[0], mpdir=%x\n", val_read_mpidr(), 0);

    /* Wait for interrupt */
    time = val_sleep_elapsed_time(timeout);

    LOG(ERROR, "\t Watchdog interrupt not triggered %d\n", time, 0);

    val_realm_return_to_host();
}
