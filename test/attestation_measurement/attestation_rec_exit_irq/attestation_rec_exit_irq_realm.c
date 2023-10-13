/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <time.h>
#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"
#include "attestation_realm.h"

void attestation_rec_exit_irq_realm(void)
{
    uint64_t *shared_flag1 = (val_get_shared_region_base() + TEST_USE_OFFSET1);
    uint64_t *shared_flag2 = (val_get_shared_region_base() + TEST_USE_OFFSET2);
    uint64_t *shared_flag3 = (val_get_shared_region_base() + TEST_USE_OFFSET3);
    uint64_t *test_shared_region_flag = (val_get_shared_region_base() + TEST_USE_OFFSET4);
    uint64_t *shared_err_flag = (val_get_shared_region_base() + TEST_USE_OFFSET5);

    val_smc_param_ts args = {0,};
    uint64_t ret;
    __attribute__((aligned (PAGE_SIZE))) uint8_t token[4096] = {0,};
    uint64_t challenge1[8] = {0xb4ea40d262abaf22,
                             0xe8d966127b6d78e2,
                             0x7ce913f20b954277,
                             0x3155ff12580f9e60,
                             0x8a3843cb95120bf6,
                             0xd52c4fca64420f43,
                             0xb75961661d52e8ce,
                             0xc7f17650fe9fca60};


    do {

        ret = val_realm_rsi_attestation_token_init((uint64_t)token, challenge1[0], challenge1[1],
                                                    challenge1[2], challenge1[3], challenge1[4],
                                                    challenge1[5], challenge1[6], challenge1[7]);

        if (ret)
        {
            *shared_err_flag = 1;
            LOG(ERROR, "\tToken init failed, ret=%x\n", ret, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto exit;
        }

        val_realm_return_to_host();

        do {
            *shared_flag1 = 1;

            args = val_realm_rsi_attestation_token_continue((uint64_t)token);
            if (args.x0 == RSI_ERROR_INCOMPLETE)
            {
                *shared_flag2 = 1;
            }

        } while (args.x0 == RSI_ERROR_INCOMPLETE);

        *shared_flag3 = 1;

        if (args.x0)
        {
            *shared_err_flag = 1;
            LOG(ERROR, "\tToken continue failed, ret=%x\n", args.x0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

    } while (*test_shared_region_flag != 1);


exit:
    val_realm_return_to_host();
}
