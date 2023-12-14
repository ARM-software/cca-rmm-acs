/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_realm_framework.h"
#include "rsi_ipa_state_set_data.h"

#define INVALID_RIPAS_VALUE 0x2
#define IPA_RIPAS_REJECT 0x0

/* Allocate base IPA which is 4KB aligned and Protected */
__attribute__((aligned (PAGE_SIZE))) static uint8_t base_gran[PAGE_SIZE];

static struct argument_store {
    uint64_t base_valid;
    uint64_t top_valid;
    uint8_t ripas_valid;
    uint64_t flags_valid;
} c_args;

struct arguments {
    uint64_t base;
    uint64_t top;
    uint8_t ripas;
    uint64_t flags;
};

static void valid_argument_prep_sequence(void)
{
    c_args.base_valid = (uint64_t)base_gran;
    c_args.top_valid = (uint64_t)base_gran + PAGE_SIZE;
    c_args.ripas_valid = RSI_EMPTY;
    c_args.flags_valid = RSI_NO_CHANGE_DESTROYED;
}

static uint64_t unaligned_prep_sequence(uint64_t addr)
{
    return addr + 1;
}

static uint64_t region_unprotected_prep_sequence(void)
{
    uint64_t ipa_width;

    ipa_width = val_realm_get_ipa_width();
    return (1ULL << (ipa_width - 1)) + PAGE_SIZE;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case BASE_UNALIGNED:
            args->base = unaligned_prep_sequence(c_args.base_valid);
            args->top = c_args.top_valid;
            args->ripas = c_args.ripas_valid;
            args->flags = c_args.flags_valid;
            break;

        case TOP_UNALLIGNED:
            args->base = c_args.base_valid;
            args->top = c_args.top_valid / 2;
            args->ripas = c_args.ripas_valid;
            args->flags = c_args.flags_valid;
            break;

        case SIZE_INVALID:
            args->base = c_args.base_valid + (2 * PAGE_SIZE);
            args->top = c_args.top_valid;
            args->ripas = c_args.ripas_valid;
            args->flags = c_args.flags_valid;
            break;

        case REGION_UNPROTECTED:
            args->base = c_args.base_valid;
            args->top = region_unprotected_prep_sequence();
            args->ripas = c_args.ripas_valid;
            args->flags = c_args.flags_valid;
            break;

        case RIPAS_INVALID:
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            args->ripas = INVALID_RIPAS_VALUE;
            args->flags = c_args.flags_valid;
            break;

        default:
            // set status to failure
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_ipa_state_set_realm(void)
{
    uint64_t ret;
    struct arguments args;
    uint64_t i;
    val_smc_param_ts cmd_ret;

    /* Prepare Valid arguments */
    valid_argument_prep_sequence();

    /* Iterate over the input */
    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "\n\tCheck %d : ", i + 1, 0);
        LOG(TEST, test_data[i].msg, 0, 0);
        LOG(TEST, "; intent id : 0x%x \n", test_data[i].label, 0);

        if (intent_to_seq(&test_data[i], &args)) {
            LOG(ERROR, "\n\tIntent to sequence failed\n", 0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto exit;
        }

        cmd_ret = val_realm_rsi_ipa_state_set(args.base, args.top,
                                          args.ripas, args.flags);
        ret = cmd_ret.x0;

        if (ret != test_data[i].status)
        {
            LOG(ERROR, "\n\tUnexpected Command Return Status\n ret status : 0x%x \n", ret, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPostive Observability\n ", 0, 0);

    cmd_ret = val_realm_rsi_ipa_state_set(c_args.base_valid, c_args.top_valid,
                                        c_args.ripas_valid, c_args.flags_valid);
    if (cmd_ret.x0)
    {
        LOG(ERROR, "\n\tRSI_IPA_STATE_SET Failed with ret = 0x%x\n", cmd_ret.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    if ((cmd_ret.x1 != c_args.top_valid) || (cmd_ret.x2 != RSI_ACCEPT)
                            || (VAL_EXTRACT_BITS(cmd_ret.x2, 1, 63) != 0))
    {
        LOG(ERROR, "\n\tPositive Observability failed.", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    /* Check RIPAS reject flow */
    cmd_ret = val_realm_rsi_ipa_state_set(IPA_RIPAS_REJECT, PAGE_SIZE,
                                        c_args.ripas_valid, c_args.flags_valid);
    if (cmd_ret.x0)
    {
        LOG(ERROR, "\n\tRSI_IPA_STATE_SET Failed with ret = 0x%x\n", cmd_ret.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    if ((cmd_ret.x2 == RSI_REJECT) && (cmd_ret.x1 != IPA_RIPAS_REJECT))
    {
        LOG(ERROR, "\n\tPositive Observability failed.", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}

