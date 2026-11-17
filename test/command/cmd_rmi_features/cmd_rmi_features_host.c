/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"

void cmd_rmi_features_host(void)
{
    uint64_t ret;
    uint64_t feature_reg;

    /* Read Feature Register 0 */
    ret = val_host_rmi_features(0, &feature_reg);

    if (ret != RMI_SUCCESS ||
        VAL_EXTRACT_BITS(feature_reg, 41, 63) != 0 ||
        VAL_EXTRACT_BITS(feature_reg, 14, 19) == 0 ||
        VAL_EXTRACT_BITS(feature_reg, 20, 25) == 0) {
        LOG(ERROR, "Invalid RMI feature register 0: ret=0x%lx value=0x%lx\n",
            ret, feature_reg);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    ret = val_host_rmi_features(1, &feature_reg);
    if (ret != RMI_SUCCESS ||
        VAL_EXTRACT_BITS(feature_reg, 17, 63) != 0 ||
        VAL_EXTRACT_BITS(feature_reg, 0, 2) == 0 ||
        VAL_EXTRACT_BITS(feature_reg, 3, 5) == 0) {
        LOG(ERROR, "Invalid RMI feature register 1: ret=0x%lx value=0x%lx\n",
            ret, feature_reg);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    ret = val_host_rmi_features(2, &feature_reg);
    if (ret != RMI_SUCCESS ||
        VAL_EXTRACT_BITS(feature_reg, 15, 63) != 0 ||
        ((VAL_EXTRACT_BITS(feature_reg, 0, 0) == RMI_FEATURE_TRUE) &&
         (VAL_EXTRACT_BITS(feature_reg, 10, 14) == 0)) ||
        ((VAL_EXTRACT_BITS(feature_reg, 0, 0) == RMI_FEATURE_FALSE) &&
         (VAL_EXTRACT_BITS(feature_reg, 10, 14) != 0))) {
        LOG(ERROR, "Invalid RMI feature register 2: ret=0x%lx value=0x%lx\n",
            ret, feature_reg);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    /* DA-dependent capabilities must be clear when DA is unsupported. */
    if ((VAL_EXTRACT_BITS(feature_reg, 0, 0) == RMI_FEATURE_FALSE) &&
        (VAL_EXTRACT_BITS(feature_reg, 1, 3) != 0)) {
        LOG(ERROR, "Invalid DA-dependent feature encoding: 0x%lx\n", feature_reg);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    ret = val_host_rmi_features(3, &feature_reg);
    if (ret != RMI_SUCCESS ||
        VAL_EXTRACT_BITS(feature_reg, 7, 63) != 0 ||
        VAL_EXTRACT_BITS(feature_reg, 4, 5) == 3 ||
        ((VAL_EXTRACT_BITS(feature_reg, 0, 3) == 0) &&
         (VAL_EXTRACT_BITS(feature_reg, 4, 5) != 0))) {
        LOG(ERROR, "Invalid RMI feature register 3: ret=0x%lx value=0x%lx\n",
            ret, feature_reg);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    ret = val_host_rmi_features(4, &feature_reg);
    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "RMI feature register 4 failed: ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    /* Query with unsupported index must return Zero */
    ret = val_host_rmi_features(5, &feature_reg);
    if (ret != RMI_SUCCESS || feature_reg != 0) {
        LOG(ERROR, "Invalid unsupported feature index result: ret=0x%lx value=0x%lx\n",
            ret, feature_reg);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
