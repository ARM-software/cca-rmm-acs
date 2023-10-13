/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "exception_common_realm.h"

#define REALM_GPRS_DATA 0xAABBCCDD
#define HOST_GPRS_DATA 0x11223344

void exception_rec_exit_hostcall_realm(void)
{
    uint32_t index = 0;
    __attribute__((aligned(PAGE_SIZE))) val_realm_rsi_host_call_t realm_host_params;

    /* Before triggering the rec exit due to hostcall fill the gprs values */
    for (index = 0; index < RSI_HOST_CALL_NR_GPRS; index++)
    {
        realm_host_params.gprs[index] = REALM_GPRS_DATA;
    }

    /* testing the rec exit due to hostcall */
    val_realm_rsi_host_params(&realm_host_params);

    /* Compare the rec_entry and hostcall GPRS values[0-30] */
    for (index = 0; index < RSI_HOST_CALL_NR_GPRS; index++)
    {
        if (realm_host_params.gprs[index] != HOST_GPRS_DATA)
        {
            LOG(ERROR, "\tGPRS values mismatch: gprs[%d]= %lx\n",
                                        index, realm_host_params.gprs[index]);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto exit;
        }
    }

exit:
    val_realm_return_to_host();
}

