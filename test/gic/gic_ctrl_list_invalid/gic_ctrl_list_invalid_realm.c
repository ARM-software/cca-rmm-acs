/*
 * Copyright (c) 2023, 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_realm_framework.h"

void gic_ctrl_list_invalid_realm(void)
{
    /* Below code is executed for REC[0] only */
    LOG(DBG, "In realm_create_realm REC[0], mpdir=%x\n", val_read_mpidr());

    val_realm_return_to_host();
}
