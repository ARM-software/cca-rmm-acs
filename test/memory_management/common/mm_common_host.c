/*
 * Copyright (c) 2023, 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "mm_common_host.h"

uint32_t val_host_init_ripas_delegate(val_host_realm_ts *realm,
                                            val_data_create_ts *data_create)
{
    uint32_t i = 0;
    uint64_t phys = data_create->target_pa;

    if (val_host_ripas_init(realm,
            data_create->ipa,
            data_create->ipa + data_create->size,
            VAL_RTT_MAX_LEVEL, data_create->rtt_alignment))
    {
        LOG(ERROR, "val_host_ripas_init failed, ipa=0x%x\n",
                data_create->ipa + i * PAGE_SIZE);
        return VAL_ERROR;
    }

    /* MAP image regions */
    while (i < (data_create->size/PAGE_SIZE))
    {
        if (val_host_rmi_granule_delegate(phys))
        {
            LOG(ERROR, "Granule delegation failed, PA=0x%x\n", phys);
            return VAL_ERROR;
        }
        phys += PAGE_SIZE;
        i++;
    }

    return VAL_SUCCESS;
}


