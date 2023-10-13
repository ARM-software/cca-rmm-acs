/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "psci_version.h"
#include "val_realm_framework.h"

void cmd_psci_version_realm(void)
{
    uint64_t version;
    uint16_t version_major, version_minor;

    /* Request for PSCI interface version from RMM */
    version = val_psci_version();

    /* Get major and minor versions*/
    version_minor = VAL_EXTRACT_BITS(version, 0, 15);
    version_major = VAL_EXTRACT_BITS(version, 16, 30);

    if (version_major < PSCI_INTERFACE_VERSION_MAJOR ||
                    version_minor < PSCI_INTERFACE_VERSION_MINOR) {
        LOG(ERROR, "\tMajor Version : 0x%x. Minor Version : 0x%x \n", version_major, version_minor);
        LOG(TEST, "\tExpected atleast - Major : 0x%x, Minor : 0x%x \n",
                    PSCI_INTERFACE_VERSION_MAJOR, PSCI_INTERFACE_VERSION_MINOR);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    /* Check for MBZ field */
    if (VAL_EXTRACT_BITS(version, 31, 63) != 0) {
        LOG(ERROR, "\tReceived non zero value for reserved field\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
    }

exit:
    val_realm_return_to_host();
}


