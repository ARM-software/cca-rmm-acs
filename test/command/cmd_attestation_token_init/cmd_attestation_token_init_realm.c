/*
 * Copyright (c) 2023, 2025-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_realm_framework.h"
#include "attestation_realm.h"

void cmd_attestation_token_init_realm(void)
{
    val_smc_param_ts args = {0,};
    uint64_t token_size = 0, size, max_size, len;
    __attribute__((aligned (PAGE_SIZE))) uint64_t token[MAX_REALM_CCA_TOKEN_SIZE/8] = {0,};
    uint8_t *granule = (uint8_t *)token;
    uint64_t challenge[8] = {0xb4ea40d262abaf22,
                             0xe8d966127b6d78e2,
                             0x7ce913f20b954277,
                             0x3155ff12580f9e60,
                             0x8a3843cb95120bf6,
                             0xd52c4fca64420f43,
                             0xb75961661d52e8ce,
                             0xc7f17650fe9fca60};
    attestation_token_ts attestation_token;

    args = val_realm_rsi_attestation_token_init(challenge[0], challenge[1],
                                                  challenge[2], challenge[3], challenge[4],
                                                 challenge[5], challenge[6], challenge[7]);
    if (args.x0)
    {
        LOG(ERROR, "Token init command failed, ret=%x\n", args.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    max_size = args.x1;
    if ((max_size == 0U) || (max_size > sizeof(token)))
    {
        LOG(ERROR, "Maximum token size cannot be represented by the ACS buffer: 0x%lx\n",
                   max_size);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    /* Check the attest state is in ATTEST_IN_PROGRESS */
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

    } while ((args.x0 == RSI_ERROR_INCOMPLETE) &&
             (granule < (uint8_t *)token + max_size));

    if (args.x0)
    {
        LOG(ERROR, "Token continue should passed, ret=%x\n", args.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    if (token_size > max_size)
    {
        LOG(ERROR, "Generated token size 0x%lx exceeds returned maximum 0x%lx\n",
                   token_size, max_size);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    if (val_attestation_verify_token(&attestation_token, challenge,
                                     ATTEST_CHALLENGE_SIZE_64, token,
                                     token_size) != VAL_SUCCESS)
    {
        LOG(ERROR, "Generated token does not contain the supplied challenge\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    val_realm_return_to_host();
}
