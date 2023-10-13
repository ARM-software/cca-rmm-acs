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

void attestation_token_init_realm(void)
{
    val_smc_param_ts args = {0,};
    uint64_t token_size, ret;
    __attribute__((aligned (PAGE_SIZE))) uint8_t token[4096] = {0,};
    uint64_t challenge1[8] = {0xb4ea40d262abaf22,
                             0xe8d966127b6d78e2,
                             0x7ce913f20b954277,
                             0x3155ff12580f9e60,
                             0x8a3843cb95120bf6,
                             0xd52c4fca64420f43,
                             0xb75961661d52e8ce,
                             0xc7f17650fe9fca60};

    uint64_t challenge2[8] = {0xc052a7e8223def88,
                             0xe8d976547b6d78e2,
                             0x7ce913f20b954277,
                             0x3155f7abc50f9e60,
                             0x8a375eabc5120bf6,
                             0xd5aaaddd64420f43,
                             0xb75961732d52e8ce,
                             0xc7f19eabce9fca60};

    attestation_token_ts attestation_token;

    ret = val_realm_rsi_attestation_token_init((uint64_t)token, challenge1[0], challenge1[1],
                                                  challenge1[2], challenge1[3], challenge1[4],
                                                 challenge1[5], challenge1[6], challenge1[7]);

    if (ret)
    {
        LOG(ERROR, "\tToken init failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    ret = val_realm_rsi_attestation_token_init((uint64_t)token, challenge2[0], challenge2[1],
                                                challenge2[2], challenge2[3], challenge2[4],
                                                challenge2[5], challenge2[6], challenge2[7]);
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

    ret = val_attestation_verify_token(&attestation_token, challenge2,
                        ATTEST_CHALLENGE_SIZE_64, token, token_size);
    if (ret != VAL_SUCCESS)
    {
        LOG(ERROR, "\tattestation token verification failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    if (val_memcmp(&challenge2, (void *)attestation_token.challenge.ptr, ATTEST_CHALLENGE_SIZE_64))
    {
        LOG(ERROR, "\tChallenge values are not same. \n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}
