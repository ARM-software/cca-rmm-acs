/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"
#include "attestation_realm.h"

void attestation_token_address_realm(void)
{
    val_smc_param_ts args = {0,};
    uint64_t ret;
    __attribute__((aligned (PAGE_SIZE))) uint8_t token1[4096] = {0,}, token2[4096] = {0,};
    uint64_t challenge[8] = {0xb4ea40d262abaf22,
                             0xe8d966127b6d78e2,
                             0x7ce913f20b954277,
                             0x3155ff12580f9e60,
                             0x8a3843cb95120bf6,
                             0xd52c4fca64420f43,
                             0xb75961661d52e8ce,
                             0xc7f17650fe9fca60};

    ret = val_realm_rsi_attestation_token_init((uint64_t)token1, challenge[0], challenge[1],
                                                  challenge[2], challenge[3], challenge[4],
                                                 challenge[5], challenge[6], challenge[7]);

    if (ret)
    {
        LOG(ERROR, "\tToken init failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    args = val_realm_rsi_attestation_token_continue((uint64_t)token2);

    if (args.x0 != RSI_ERROR_INPUT)
    {
        LOG(ERROR, "\tToken continue should failed, return error code=%x\n", args.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}
