/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
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
    uint64_t addr_valid;
} c_args;

struct arguments {
    uint64_t addr;
};

static void valid_argument_prep_sequence(void)
{
    c_args.addr_valid = (uint64_t)target_page;
}

static uint64_t unaligned_prep_sequence(uint64_t addr)
{
    return addr + 1;
}

static uint64_t address_unprotected_prep_sequence(void)
{
    uint64_t ipa_width;

    ipa_width = val_realm_get_ipa_width();
    return 1ULL << (ipa_width - 1);
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case ADDR_UNALIGNED:
            args->addr = unaligned_prep_sequence(c_args.addr_valid);
            break;

        case ADDR_UNPROTECTED:
            args->addr = address_unprotected_prep_sequence();
            break;

        default:
            // set status to failure
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
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
        LOG(TEST, "\n\tCheck %d : ", i + 1, 0);
        LOG(TEST, test_data[i].msg, 0, 0);
        LOG(TEST, "; intent id : 0x%x \n", test_data[i].label, 0);


        if (intent_to_seq(&test_data[i], &args)) {
            LOG(ERROR, "\n\tIntent to sequence failed\n", 0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto exit;
        }

        cmd_ret = val_realm_rsi_ipa_state_get(args.addr);
        ret = cmd_ret.x0;

        if (ret != test_data[i].status)
        {
            LOG(ERROR, "\n\tUnexpected Command Return Status\n ret status : 0x%x \n", ret, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);

    cmd_ret = val_realm_rsi_ipa_state_get(c_args.addr_valid);
    if (cmd_ret.x0)
    {
        LOG(ERROR, "\n\tPositive Observability failed. Ret = 0x%x\n", cmd_ret.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}


