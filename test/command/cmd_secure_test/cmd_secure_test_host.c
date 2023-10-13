/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"

#define DATA_PATTERN_1 0x12345678
#define DATA_PATTERN_2 0x11223344

void cmd_secure_test_host(void)
{
#ifndef SECURE_TEST_ENABLE
    val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
    goto exit;
#else
    uint64_t *test_shared_region = (val_get_shared_region_base() + TEST_USE_OFFSET1);

    /* Update shared region with test pattern */
    *test_shared_region = DATA_PATTERN_1;

    /* Execute secure payload  */
    if (val_host_execute_secure_payload())
    {
        LOG(ERROR, "\tval_execute_secure_payload() failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    /* Compare data pattern with expected value */
    if (*test_shared_region == DATA_PATTERN_2)
        val_set_status(RESULT_PASS(VAL_SUCCESS));
    else
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));

#endif
exit:
    return;
}
