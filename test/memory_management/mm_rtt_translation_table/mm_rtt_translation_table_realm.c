/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_realm_framework.h"

void mm_rtt_translation_table_realm(void)
{
    uint64_t mmfr0;

    /* Below code is executed for REC[0] only */
    LOG(DBG, "\tIn realm_create_realm REC[0], mpdir=%x\n", val_read_mpidr(), 0);

    mmfr0 = val_id_aa64mmfr0_el1_read();
    /* Check that 4KB granule is supported. */
    if (!((VAL_EXTRACT_BITS(mmfr0, 28, 31) == 0x0) ||
         (VAL_EXTRACT_BITS(mmfr0, 28, 31) == 0x1)))
    {
        LOG(ERROR, "\tPlatform not supported 4KB Granule\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
    }

    val_realm_return_to_host();
}
