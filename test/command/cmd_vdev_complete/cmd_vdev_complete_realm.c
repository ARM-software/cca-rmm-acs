/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"

#define PAGE_SIZE_ALIGNED __attribute__((aligned (PAGE_SIZE)))

void cmd_vdev_complete_realm(void)
{
    uint64_t feature_reg;
    val_smc_param_ts args;
    val_realm_rsi_host_call_t *gv_realm_host_call;
    uint64_t vdev_id;
    PAGE_SIZE_ALIGNED val_realm_rsi_vdev_info_ts vdev_info = {0};

    /* Read Feature Register 0 and check for DA support */
    val_realm_rsi_features(0, &feature_reg);
    if (VAL_EXTRACT_BITS(feature_reg, 0, 0) == 0) {
        LOG(ERROR, "DA feature not supported.\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto exit;
    }

    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    vdev_id = gv_realm_host_call->gprs[1];

    args = val_realm_rsi_vdev_get_info(vdev_id, (uint64_t)&vdev_info);
    if (args.x0)
    {
        LOG(ERROR, "VDEV_GET_INFO failed, ret %x.\n", args.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

exit:
    val_realm_return_to_host();

}
