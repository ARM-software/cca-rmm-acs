/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "exception_common_realm.h"
void exception_realm_unsupported_smc_realm(void)
{
    uint64_t retVal = 0;
    uint8_t buff[100];

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* smc call with invalid RSI/PSCI call */
    retVal = val_smc_call(RMI_REALM_CREATE, (uint64_t)buff,
                                      0, 0, 0, 0, 0, 0, 0, 0, 0).x0;

    if (retVal == VAL_SMC_NOT_SUPPORTED)
    {
        LOG(TEST, "Verified : SMCCC_NOT_SUPPORTED by RMI_REALM_CREATE SMC\n");
    } else {
        LOG(ERROR, "Not Verified SMCCC_NOT_SUPPORTED, ret=%d\n",\
                          (uint64_t)retVal);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto test_exit;
    }

test_exit:
    val_realm_return_to_host();
}

