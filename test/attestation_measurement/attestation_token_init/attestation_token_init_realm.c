/*
 * Copyright (c) 2023, 2025, Arm Limited or its affiliates. All rights reserved.
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
    uint64_t token_size = 0, ret, size, max_size, len;
    __attribute__((aligned (PAGE_SIZE))) uint64_t token[MAX_REALM_CCA_TOKEN_SIZE/8] = {0,};
    uint64_t *granule = token;

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

    args = val_realm_rsi_attestation_token_init(challenge1[0], challenge1[1],
                                                  challenge1[2], challenge1[3], challenge1[4],
                                                 challenge1[5], challenge1[6], challenge1[7]);

    if (args.x0)
    {
        LOG(ERROR, "Token init failed, ret=%x\n", args.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    args = val_realm_rsi_attestation_token_init(challenge2[0], challenge2[1],
                                                challenge2[2], challenge2[3], challenge2[4],
                                                challenge2[5], challenge2[6], challenge2[7]);
    if (args.x0)
    {
        LOG(ERROR, "Token init failed, ret=%x\n", args.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    max_size = args.x1;

    do {
        uint64_t offset = 0;
        do {
            size = PAGE_SIZE - offset;
            args = val_realm_rsi_attestation_token_continue((uint64_t)granule, offset, size, &len);
            offset += len;
            token_size = token_size + len;
        } while (args.x0 == RSI_ERROR_INCOMPLETE && offset < PAGE_SIZE);

        if (args.x0 == RSI_ERROR_INCOMPLETE)
        {
            granule += PAGE_SIZE;
        }

    } while ((args.x0 == RSI_ERROR_INCOMPLETE) && (granule < token + max_size));

    if (args.x0)
    {
        LOG(ERROR, "Token continue failed, ret=%x\n", args.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    ret = val_attestation_verify_token(&attestation_token, challenge2,
                        ATTEST_CHALLENGE_SIZE_64, token, token_size);
    if (ret != VAL_SUCCESS)
    {
        LOG(ERROR, "attestation token verification failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    if (val_memcmp(&challenge2, (void *)attestation_token.challenge.ptr, ATTEST_CHALLENGE_SIZE_64))
    {
        LOG(ERROR, "Challenge values are not same. \n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}
