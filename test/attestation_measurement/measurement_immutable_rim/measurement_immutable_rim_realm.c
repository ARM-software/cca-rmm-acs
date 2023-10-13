/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"

void measurement_immutable_rim_realm(void)
{
    val_smc_param_ts args1, args2;

    /* Get the RIM value after activating realm */
    args1 = val_realm_rsi_measurement_read(0);

    val_realm_return_to_host();

    /* Get the RIM value after adding and destroying granules */
    args2 = val_realm_rsi_measurement_read(0);

    /* Compare both RIM to verify RIM is immutable */
    if (val_memcmp(&args1, &args2, sizeof(args1))) {
        LOG(ERROR, "\t Both RIM values are not same \n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}
