/*
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"

#define VER(maj, min) (((uint64_t)(maj << 16)) | ((uint64_t)min))

void cmd_rmi_version_host(void)
{
    val_host_rmi_version_ts rmi_ver_out;
    uint64_t version_major, version_minor;
    uint64_t ret;

    /* Request for RMI ABI version from RMM */
    ret = val_host_rmi_version(VER(RMI_ABI_VERSION_MAJOR, RMI_ABI_VERSION_MINOR), &rmi_ver_out);

    /* Check for MBZ filed */
    if ((VAL_EXTRACT_BITS(rmi_ver_out.lower, 31, 63) != 0) ||
        (VAL_EXTRACT_BITS(rmi_ver_out.higher, 31, 63) != 0))  {
        LOG(ERROR, "\tReceived non zero value for reserved field\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    /* Upon receiving RMI_SUCCESS check if lower version == requested version */
    if (!ret) {
        /* Get major and minor versions*/
        version_minor = VAL_EXTRACT_BITS(rmi_ver_out.lower, 0, 15);
        version_major = VAL_EXTRACT_BITS(rmi_ver_out.lower, 16, 30);

        if (version_major != RMI_ABI_VERSION_MAJOR || version_minor != RMI_ABI_VERSION_MINOR) {
            LOG(ERROR, "\tRMI lower version : %d.%d \n", version_major, version_minor);
            LOG(TEST, "\tExpected version : %d.%d \n", RMI_ABI_VERSION_MAJOR,
                                                       RMI_ABI_VERSION_MINOR);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }
    }

    else {
        LOG(TEST, "\tINFO: RMI_ABI lower version : %d.%d \n",
                            VAL_EXTRACT_BITS(rmi_ver_out.lower, 16, 30),
                            VAL_EXTRACT_BITS(rmi_ver_out.lower, 0, 15));
        LOG(TEST, "\tINFO: RMI_ABI higher version : %d.%d \n",
                            VAL_EXTRACT_BITS(rmi_ver_out.higher, 16, 30),
                            VAL_EXTRACT_BITS(rmi_ver_out.higher, 0, 15));
        LOG(ERROR, "\tRMI ABI version is incompatibe with ACS, Expected : %d.%d \n",
                                                 RMI_ABI_VERSION_MAJOR, RMI_ABI_VERSION_MINOR);
        assert(ret == RMI_SUCCESS);
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}


