/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_secure_framework.h"

#define DATA_PATTERN_1 0x12345678
#define DATA_PATTERN_2 0x11223344

void cmd_secure_test_secure(void)
{
    uint64_t *test_shared_region = (val_get_shared_region_base() + TEST_USE_OFFSET1);

    LOG(ALWAYS, "\tIn secure_test_secure\n", 0, 0);

    /* Compare data pattern with expected value sent by host */
    if (*test_shared_region != DATA_PATTERN_1)
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(201)));

    /* Update shared region with different test pattern */
    *test_shared_region = DATA_PATTERN_2;

    val_secure_return_to_host();
}
