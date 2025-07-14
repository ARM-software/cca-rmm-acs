/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_realm_framework.h"
#include "rsi_ipa_state_get_data.h"

/* 4KB aligned and Protected IPA whose state is being read */
__attribute__((aligned (PAGE_SIZE))) static uint8_t target_page[PAGE_SIZE];

static struct argument_store {
    uint64_t base_valid;
    uint64_t top_valid;
} c_args;

struct arguments {
    uint64_t base;
    uint64_t top;
};

static void valid_argument_prep_sequence(void)
{
    c_args.base_valid = (uint64_t)target_page;
    c_args.top_valid = c_args.base_valid + PAGE_SIZE;
}

static uint64_t unaligned_prep_sequence(uint64_t addr)
{
    return addr + 1;
}

static uint64_t address_unprotected_prep_sequence(void)
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
            break;

        case TOP_UNALIGNED:
            args->base = c_args.base_valid;
            args->top = unaligned_prep_sequence(c_args.top_valid);
            break;

        case SIZE_INVALID:
            args->base = c_args.base_valid;
            args->top = args->base - PAGE_SIZE;
            break;

        case REGION_UNPROTECTED:
            args->base = c_args.base_valid;
            args->top = address_unprotected_prep_sequence();
            break;

        default:
            // set status to failure
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_ipa_state_get_realm(void)
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
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
              i + 1, test_data[i].msg, test_data[i].label);


        if (intent_to_seq(&test_data[i], &args)) {
            LOG(ERROR, "Intent to sequence failed\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto exit;
        }

        cmd_ret = val_realm_rsi_ipa_state_get(args.base, args.top);
        ret = cmd_ret.x0;

        if (ret != test_data[i].status)
        {
            LOG(ERROR, "Unexpected Command Return Status ret status : 0x%x \n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);

    cmd_ret = val_realm_rsi_ipa_state_get(c_args.base_valid, c_args.top_valid);
    if (cmd_ret.x0)
    {
        LOG(ERROR, "RSI_IPA_STATE_GET Failed with ret = 0x%x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    if ((cmd_ret.x2 != RSI_RAM) || (VAL_EXTRACT_BITS(cmd_ret.x2, 8, 63) != 0)) {
        LOG(ERROR, "Positive Observability failed.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}


