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

#define MEASUREMENT_SIZE 32

void attestation_rem_extend_check_realm_token_realm(void)
{
    val_smc_param_ts args = {0}, zero_ref = {0};
    uint64_t token_size, ret;
    __attribute__((aligned (PAGE_SIZE))) uint8_t token[4096] = {0,};
    uint64_t challenge[8] = {0xb4ea40d262abaf22,
                             0xe8d966127b6d78e2,
                             0x7ce913f20b954277,
                             0x3155ff12580f9e60,
                             0x8a3843cb95120bf6,
                             0xd52c4fca64420f43,
                             0xb75961661d52e8ce,
                             0xc7f17650fe9fca60};
    attestation_token_ts attestation_token;

    /* Extend REM for index 1 */
    ret = val_realm_rsi_measurement_extend(1, MEASUREMENT_SIZE,
					      0x05a9bf223fedf80a,
                                              0x9d0da5f73f5c191a,
                                              0x665bf4a0a4a3e608,
                                              0xf2f9e7d5ff23959c,
                                              0, 0, 0, 0);
    if (ret)
    {
        LOG(ERROR, "\tRealm RSI MEASUREMENT EXTEND failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    ret = val_realm_rsi_attestation_token_init((uint64_t)token, challenge[0], challenge[1],
                                                  challenge[2], challenge[3], challenge[4],
                                                 challenge[5], challenge[6], challenge[7]);
    if (ret)
    {
        LOG(ERROR, "\tToken init failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    do {
        args = val_realm_rsi_attestation_token_continue((uint64_t)token);
    } while (args.x0 == RSI_ERROR_INCOMPLETE);

    if (args.x0)
    {
        LOG(ERROR, "\tToken continue failed, ret=%x\n", args.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    token_size = args.x1;
    if (token_size > ATTEST_MAX_TOKEN_SIZE)
    {
        LOG(ERROR, "\tattestation token should not be larger than 4KB\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    ret = val_attestation_verify_token(&attestation_token, challenge,
                        ATTEST_CHALLENGE_SIZE_64, token, token_size);
    if (ret != VAL_SUCCESS)
    {
        LOG(ERROR, "\tattestation token verification failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    /* Compare REM for index 1 with zero initialize structure, if REM is zero it should fail */
    if (!val_memcmp(&zero_ref, (void *)attestation_token.rem[0].ptr, attestation_token.rem[0].len))
    {
        LOG(ERROR, "\t rem is zero. \n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}
