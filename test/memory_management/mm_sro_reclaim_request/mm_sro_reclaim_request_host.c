/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "mm_common_host.h"
#include "val_host_sro.h"

void mm_sro_reclaim_request_host(void)
{
    mm_reclaim_sro_ts sro;
    uint64_t ret;

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

    ret = mm_complete_reclaim_sro(&sro);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "SRO cleanup completion failed, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        mm_cleanup_reclaim_sro(&sro);
        return;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));
    mm_cleanup_reclaim_sro(&sro);
}
