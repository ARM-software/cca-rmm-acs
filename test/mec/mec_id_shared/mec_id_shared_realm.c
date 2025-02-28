/*
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_realm_framework.h"

#define VER(maj, min) (((uint64_t)(maj << 16)) | ((uint64_t)min))

void mec_id_shared_realm(void)
{
    val_realm_rsi_version_ts rsi_ver_out;
    uint64_t version_major, version_minor;
    uint64_t ret;

    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t host_call_struct = {0};

    /* Request for RSI ABI version from RMM */
    ret = val_realm_rsi_version(VER(RSI_ABI_VERSION_MAJOR, RSI_ABI_VERSION_MINOR), &rsi_ver_out);

    /* Upon receiving RSI_SUCCESS check if lower version == requested version */
    if (!ret) {

        /* Get major and minor versions*/
        version_minor = VAL_EXTRACT_BITS(rsi_ver_out.lower, 0, 15);
        version_major = VAL_EXTRACT_BITS(rsi_ver_out.lower, 16, 30);

        if (version_major != RSI_ABI_VERSION_MAJOR || version_minor != RSI_ABI_VERSION_MINOR) {
            LOG(ERROR, "\tRSI lower version : %d.%d \n", version_major, version_minor);
            LOG(TEST, "\tExpected version : %d.%d \n", RSI_ABI_VERSION_MAJOR,
                                                       RSI_ABI_VERSION_MINOR);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
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
    val_realm_rsi_host_call_struct((uint64_t)&host_call_struct);
}


