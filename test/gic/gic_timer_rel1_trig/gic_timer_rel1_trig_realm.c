/*
 * Copyright (c) 2023, 2025 Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"
#include "val_timer.h"

#define TIMEOUT     1

void gic_timer_rel1_trig_realm(void)
{
    /* Below code is executed for REC[0] only */
    LOG(DBG, "\tIn realm_create_realm REC[0], mpdir=%x\n", val_read_mpidr(), 0);
    val_timer_set_phy_el1(TIMEOUT, false);

    val_sleep_elapsed_time(TIMEOUT + 1);

    val_realm_return_to_host();
}
