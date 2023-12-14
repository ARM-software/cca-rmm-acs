/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "rsi_version.h"
#include "val_realm_framework.h"

#define VER(maj, min) (((uint64_t)(maj << 16)) | ((uint64_t)min))

void cmd_rsi_version_realm(void)
{
    val_realm_rsi_version_ts rsi_ver_out;
    uint64_t version_major, version_minor;
    uint64_t ret;

    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t host_call_struct = {0};

    /* Request for RSI ABI version from RMM */
    ret = val_realm_rsi_version(VER(RSI_ABI_VERSION_MAJOR, RSI_ABI_VERSION_MINOR), &rsi_ver_out);

    /* Check for MBZ filed */
    if ((VAL_EXTRACT_BITS(rsi_ver_out.lower, 31, 63) != 0) ||
        (VAL_EXTRACT_BITS(rsi_ver_out.higher, 31, 63) != 0))  {
        LOG(ERROR, "\tReceived non zero value for reserved field\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    /* Upon receiving RSI_SUCCESS check if lower version == requested version */
    if (!ret) {

        /* Get major and minor versions*/
        version_minor = VAL_EXTRACT_BITS(rsi_ver_out.lower, 0, 15);
        version_major = VAL_EXTRACT_BITS(rsi_ver_out.lower, 16, 30);

        if (version_major != RSI_ABI_VERSION_MAJOR || version_minor != RSI_ABI_VERSION_MINOR) {
            LOG(ERROR, "\tRSI lower version : %d.%d \n", version_major, version_minor);
            LOG(TEST, "\tExpected version : %d.%d \n", RSI_ABI_VERSION_MAJOR,
                                                       RSI_ABI_VERSION_MINOR);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }
    }

    else {

        LOG(TEST, "\tINFO: RSI_ABI lower version : %d.%d \n",
                            VAL_EXTRACT_BITS(rsi_ver_out.lower, 16, 30),
                            VAL_EXTRACT_BITS(rsi_ver_out.lower, 0, 15));
        LOG(TEST, "\tINFO: RSI_ABI higher version : %d.%d \n",
                            VAL_EXTRACT_BITS(rsi_ver_out.higher, 16, 30),
                            VAL_EXTRACT_BITS(rsi_ver_out.higher, 0, 15));
        LOG(ERROR, "\tRSI ABI version is incompatibe with ACS, Expected : %d.%d \n",
                                                 RSI_ABI_VERSION_MAJOR, RSI_ABI_VERSION_MINOR);
    }

exit:

    /* return RSI return value to host */
    host_call_struct.imm = VAL_SWITCH_TO_HOST;
    host_call_struct.gprs[0] = ret;
    val_realm_rsi_host_call_struct((uint64_t)&host_call_struct);
}


