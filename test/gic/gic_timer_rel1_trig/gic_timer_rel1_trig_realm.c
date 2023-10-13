/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"
#include "val_timer.h"

void gic_timer_rel1_trig_realm(void)
{
    uint64_t timeout = 0x100000000000000;

    /* Below code is executed for REC[0] only */
    LOG(DBG, "\tIn realm_create_realm REC[0], mpdir=%x\n", val_read_mpidr(), 0);
    val_timer_set_phy_el1(1, false);

    while (timeout--);

    val_realm_return_to_host();
}
