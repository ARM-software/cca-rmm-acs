/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"

void cmd_rmi_features_host(void)
{
    uint64_t feature_reg;

    /* Read Feature Register 0*/
    val_host_rmi_features(0, &feature_reg);

    /* RmiFeatureTegister0[30:63] Must be Zero  */
    if (VAL_EXTRACT_BITS(feature_reg, 30, 63) != 0) {
        LOG(ERROR, "\tReceived non zero value \n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    /* Query with index!=0 must return Zero */
    val_host_rmi_features(1, &feature_reg);

    if (feature_reg != 0) {
        LOG(ERROR, "Read non zero value \n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}


