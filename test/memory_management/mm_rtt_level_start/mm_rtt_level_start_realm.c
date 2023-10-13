/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_realm_framework.h"
#include "rtt_level_start.h"
#include "val_realm_memory.h"
#include "val_realm_rsi.h"
#include "val_realm_memory.h"

#define REALM_DATA   0xAABB1122

void mm_rtt_level_start_realm(void)
{
    uint64_t ipa_width;
    uint32_t i;
    val_memory_region_descriptor_ts mem_desc;
    uint64_t *addr;

    /* Below code is executed for REC[0] only */
    LOG(DBG, "\tIn realm_create_realm REC[0], mpdir=%x\n", val_read_mpidr(), 0);
    ipa_width = val_realm_get_ipa_width();

    for (i = 0; i < 23; i++)
    {
        if (ipa_width != rtt_sl_start[i][0])
            continue;

        mem_desc.virtual_address = rtt_sl_start[i][1];
        mem_desc.physical_address = rtt_sl_start[i][1];
        mem_desc.length = PAGE_SIZE;
        mem_desc.attributes = MT_RW_DATA | MT_NS;
        if (val_realm_pgt_create(&mem_desc))
        {
            LOG(ERROR, "\tVA to PA mapping failed\n", 0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto exit;
        }

        /* Write to memory location */
        addr = (uint64_t *)rtt_sl_start[i][1];
        *addr = REALM_DATA;

        /* Read and Compare the data */
        if (*addr != REALM_DATA)
        {
            LOG(ERROR, "\tData mismatch\n", 0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }
    }

exit:
    val_realm_return_to_host();
}
