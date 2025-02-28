/*
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_lfa.h"

#define LFA_MAJOR_VERSION  0x1
#define LFA_MINOR_VERSION  0x0

void lfa_test_host(void)
{
    uint64_t lfa_info_selector;
    val_smc_param_ts args;
    uint64_t lfa_num_components;
    uint64_t fw_seq_id;
    uint32_t status;

    args = val_host_lfa_version();
    if ((int64_t)args.x0 < 0)
    {
        LOG(ERROR, "lfa_version failed: %d\n", args.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    if (((uint32_t)(args.x0 >> 16) != LFA_MAJOR_VERSION) ||
                ((uint32_t)(args.x0 & 0xffff) != LFA_MINOR_VERSION))
    {
        LOG(ERROR, "lfa_version : %x\n", args.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    LOG(DBG, "lfa_version : %x\n", args.x0, 0);

    lfa_info_selector = 0;
    args = val_host_lfa_get_info(lfa_info_selector);
    if (args.x0 != 0)
    {
        LOG(ERROR, "lfa_get_info failed: %d\n", args.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    LOG(DBG, "lfa_num_components %x\n", args.x1, 0);
    lfa_num_components = args.x1;

    for (fw_seq_id = 0; fw_seq_id < lfa_num_components; fw_seq_id++)
    {
        args = val_host_lfa_get_inventory(fw_seq_id);
        if (args.x0 != 0)
        {
            LOG(ERROR, "lfa_get_inventory failed: %d\n", args.x0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }
        LOG(DBG, "ID[%d]: guid0:%x ", fw_seq_id, args.x1);
        LOG(DBG, "guid1:%x flags:%x\n", args.x2, args.x3);
        /* Find the RMM firmware sequence id */
        if ((PLATFORM_LFA_RMM_GUID0 == args.x1) && (PLATFORM_LFA_RMM_GUID1 == args.x2))
            break;
    }

    args = val_host_lfa_prime(fw_seq_id);
    status = (uint32_t)args.x0;
    if ((int32_t)status != LFA_WRONG_STATE)
    {
        LOG(ERROR, "lfa prime must fail with wrong state: %d\n", args.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
exit:
    return;
}
