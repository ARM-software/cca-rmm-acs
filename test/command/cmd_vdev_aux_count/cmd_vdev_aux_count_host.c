/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"

void cmd_vdev_aux_count_host(void)
{
    val_smc_param_ts args;
    val_host_pdev_flags_ts pdev_flags;
    val_host_vdev_flags_ts vdev_flags;
    uint64_t flags_pdev, flags_vdev;

    /* Skip if RMM do not support DA */
    if (!val_host_rmm_supports_da())
    {
        LOG(ALWAYS, "DA feature not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto exit;
    }

    val_memset(&pdev_flags, 0, sizeof(pdev_flags));
    val_memset(&vdev_flags, 0, sizeof(vdev_flags));

    pdev_flags.spdm = RMI_SPDM_TRUE;
    pdev_flags.ncoh_ide = RMI_IDE_TRUE;

    vdev_flags.vsmmu = RMI_FEATURE_FALSE;

    val_memcpy(&flags_pdev, &pdev_flags, sizeof(pdev_flags));
    val_memcpy(&flags_vdev, &vdev_flags, sizeof(vdev_flags));

    args = val_host_rmi_vdev_aux_count(flags_pdev, flags_vdev);
    if (args.x0) {
        LOG(ERROR, "VDEV AUX count ABI failed, ret value is: %x.\n", args.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    if (args.x1 > MAX_VDEV_AUX_GRANULES) {
        LOG(ERROR, "VDEV AUX count greater than %d is not valid, return aux count is: %x.\n",
                                                                MAX_VDEV_AUX_GRANULES, args.x1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}