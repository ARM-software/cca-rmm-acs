/*
 * Copyright (c) 2023, 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_realm_framework.h"
#include "rsi_attestation_token_continue_data.h"

__attribute__((aligned (PAGE_SIZE))) uint64_t token[MAX_REALM_CCA_TOKEN_SIZE/8] = {0,};
uint64_t max_size;
static struct argument_store {
    uint64_t addr_valid;
    uint64_t offset_valid;
    uint64_t size_valid;
} c_args;

struct arguments {
    uint64_t addr;
    uint64_t offset;
    uint64_t size;
};

static uint64_t valid_argument_prep_sequence(void)
{
    val_smc_param_ts args = {0,};
    uint64_t offset = 0;
    uint64_t *granule = token;
    uint64_t challenge[8] = {0xb4ea40d262abaf22,
                             0xe8d966127b6d78e2,
                             0x7ce913f20b954277,
                             0x3155ff12580f9e60,
                             0x8a3843cb95120bf6,
                             0xd52c4fca64420f43,
                             0xb75961661d52e8ce,
                             0xc7f17650fe9fca60};

    args = val_realm_rsi_attestation_token_init(challenge[0], challenge[1],
                                                challenge[2], challenge[3], challenge[4],
                                                challenge[5], challenge[6], challenge[7]);
    if (args.x0)
    {
        LOG(ERROR, "Token init failed, ret=%x\n", args.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    max_size = args.x1;
    c_args.addr_valid = (uint64_t)granule;
    c_args.offset_valid = offset;
    c_args.size_valid = PAGE_SIZE - offset;

    return VAL_SUCCESS;

}

static uint64_t unaligned_addr_prep_sequence(uint64_t addr)
{
    return addr + 1;
}

static uint64_t address_unprotected_prep_sequence(void)
{
    uint64_t ipa_width;

    ipa_width = val_realm_get_ipa_width();
    return 1ULL << (ipa_width - 1);
}

static uint64_t offset_bound_prep_sequence(uint64_t offset)
{
    return offset + PAGE_SIZE + 1;
}

static uint64_t size_overflow_prep_sequence(void)
{
    return ~0ULL;
}

static uint64_t size_bound_prep_sequence(uint64_t size)
{
    return size + 1;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case ADDR_ALIGN:
            args->addr = unaligned_addr_prep_sequence(c_args.addr_valid);
            args->offset = c_args.offset_valid;
            args->size = c_args.size_valid;
            break;

        case ADDR_BOUND:
            args->addr = address_unprotected_prep_sequence();
            args->offset = c_args.offset_valid;
            args->size = c_args.size_valid;
            break;

        case OFFSET_BOUND:
            args->addr = c_args.addr_valid;
            args->offset = offset_bound_prep_sequence(c_args.offset_valid);
            args->size = c_args.size_valid;
            break;

        case SIZE_OVERFLOW:
            args->addr = c_args.addr_valid;
            args->offset = c_args.offset_valid;
            args->size = size_overflow_prep_sequence();
            break;

        case SIZE_BOUND:
            args->addr = c_args.addr_valid;
            args->offset = c_args.offset_valid;
            args->size = size_bound_prep_sequence(c_args.size_valid);
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_attestation_token_continue_realm(void)
{
    uint64_t ret, size;
    struct arguments args;
    uint64_t i, len;
    val_smc_param_ts cmd_ret;

    /* Prepare valid arguments */
    if (valid_argument_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }
    /* Iterate over the input */
    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
              i + 1, test_data[i].msg, test_data[i].label);

        if (intent_to_seq(&test_data[i], &args)) {
            LOG(ERROR, "Intent to sequence failed\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        cmd_ret = val_realm_rsi_attestation_token_continue(args.addr, args.offset, args.size, &len);
        ret = cmd_ret.x0;
        if (ret != test_data[i].status)
        {
            LOG(ERROR, "Unexpected Command Return Status ret status : 0x%x \n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);

    do {
        uint64_t offset = c_args.offset_valid;
        do {
            size = PAGE_SIZE - offset;
            cmd_ret = val_realm_rsi_attestation_token_continue(c_args.addr_valid, offset,
                                                                             size, &len);
            offset += len;
        } while (cmd_ret.x0 == RSI_ERROR_INCOMPLETE && offset < PAGE_SIZE);

        if (cmd_ret.x0 == RSI_ERROR_INCOMPLETE)
        {
            c_args.addr_valid += PAGE_SIZE;
        }

    } while ((cmd_ret.x0 == RSI_ERROR_INCOMPLETE) &&
             (c_args.addr_valid < (uint64_t)token + max_size));

    if (cmd_ret.x0)
    {
        LOG(ERROR, "Positive Observability Check failed, ret=%x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}

