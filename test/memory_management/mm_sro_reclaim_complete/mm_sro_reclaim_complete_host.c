/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "mm_common_host.h"
#include "val_host_sro.h"

void mm_sro_reclaim_complete_host(void)
{
    mm_reclaim_sro_ts sro;
    uint64_t list_addr;
    uint64_t ret;
    uint64_t i;
    uint32_t saw_reclaim_complete = 0U;

    ret = mm_prepare_rec_destroy_reclaim_sro(&sro);
    if (ret == VAL_SKIP_CHECK)
    {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        mm_cleanup_reclaim_sro(&sro);
        return;
    }
    if (ret != VAL_SUCCESS)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        mm_cleanup_reclaim_sro(&sro);
        return;
    }

    list_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (list_addr == 0)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        mm_cleanup_reclaim_sro(&sro);
        return;
    }

    for (i = 0; i < VAL_HOST_SRO_MAX_ITERATIONS; i++)
    {
        ret = mm_reclaim_sro_once(&sro, list_addr,
                                  VAL_HOST_SRO_LIST_MAX_ENTRIES);
        if (ret != VAL_SUCCESS)
        {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto out;
        }

        if (VAL_HOST_SRO_MEM_REQ(sro.args.x0) == RMI_OP_MEM_REQ_NONE)
        {
            saw_reclaim_complete = 1U;
            break;
        }

        if (VAL_HOST_SRO_MEM_REQ(sro.args.x0) != RMI_OP_MEM_REQ_RECLAIM)
        {
            LOG(ERROR, "Unexpected reclaim response, ret=0x%lx\n", sro.args.x0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto out;
        }
    }

    if ((i == VAL_HOST_SRO_MAX_ITERATIONS) || (saw_reclaim_complete == 0U))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto out;
    }

    ret = mm_complete_reclaim_sro(&sro);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "SRO completion failed, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto out;
    }

    sro.handle = 0;
    val_set_status(RESULT_PASS(VAL_SUCCESS));

out:
    val_host_mem_free((void *)list_addr);
    mm_cleanup_reclaim_sro(&sro);
}
