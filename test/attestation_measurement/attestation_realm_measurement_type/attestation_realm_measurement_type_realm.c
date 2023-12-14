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

void attestation_realm_measurement_type_realm(void)
{
    val_smc_param_ts args = {0,};
    uint64_t token_size = 0, ret, size, max_size, len;
    __attribute__((aligned (PAGE_SIZE))) uint64_t token[MAX_REALM_CCA_TOKEN_SIZE/8] = {0,};
    uint64_t *granule = token;
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
        LOG(ERROR, "\tToken init failed, ret=%x\n", args.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
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
        LOG(ERROR, "\tToken continue failed, ret=%x\n", args.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    ret = val_attestation_verify_token(&attestation_token, challenge,
                        ATTEST_CHALLENGE_SIZE_64, token, token_size);
    if (ret != VAL_SUCCESS)
    {
        LOG(ERROR, "\tattestation token verification failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    if (attestation_token.realm_initial_measurement.len != CCA_BYTE_SIZE_32 &&
        attestation_token.realm_initial_measurement.len != CCA_BYTE_SIZE_48 &&
        attestation_token.realm_initial_measurement.len != CCA_BYTE_SIZE_64)
    {
        LOG(ERROR, "\tRealm initial measurement is not in given size format.", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}
