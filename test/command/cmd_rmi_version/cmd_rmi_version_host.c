/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_version.h"

void cmd_rmi_version_host(void)
{
    uint64_t version;
    uint16_t version_major, version_minor;

    /* Request for RMI ABI version from RMM */
    version = val_host_rmi_version();

    /* Get major and minor versions*/
    version_minor = VAL_EXTRACT_BITS(version, 0, 15);
    version_major = VAL_EXTRACT_BITS(version, 16, 30);

    if (version_major != RMI_ABI_VERSION_MAJOR || version_minor != RMI_ABI_VERSION_MINOR) {
        LOG(ERROR, "\tMajor Version : 0x%x. Minor Version : 0x%x \n", version_major, version_minor);
        LOG(TEST, "\tExpected version - Major : 0x%x, Minor : 0x%x \n",
                                                RMI_ABI_VERSION_MAJOR, RMI_ABI_VERSION_MINOR);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    /* Check for MBZ filed */
    if (VAL_EXTRACT_BITS(version, 31, 63) != 0) {
        LOG(ERROR, "\tReceived non zero value for reserved field\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}


