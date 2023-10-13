/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_realm_framework.h"
#include "rsi_host_call_data.h"

__attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t realm_host_call = {0};

static struct argument_store {
    uint64_t addr_valid;
} c_args;

struct arguments {
    uint64_t addr;
};

static void valid_argument_prep_sequence(void)
{
    c_args.addr_valid = (uint64_t)&realm_host_call;
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

void cmd_host_call_realm(void)
{
    uint64_t ret;
    struct arguments args;
    uint64_t i;

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

        ret = val_realm_rsi_host_call_struct(args.addr);
        if (ret != test_data[i].status)
        {
            LOG(ERROR, "\n\tUnexpected Command Return Status\n ret status : 0x%x \n", ret, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);

    realm_host_call.imm = VAL_SWITCH_TO_HOST;
    ret = val_realm_rsi_host_call_struct(c_args.addr_valid);
    if (ret)
    {
        LOG(ERROR, "\n\tUnexpected Command Return Status\n ret status : 0x%x \n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    /* Check the value of host call structure gprs[1] value is FF */
    if (realm_host_call.gprs[1] != 0xFF)
    {
        LOG(ERROR, "GPRS 1=  0x%x \n", realm_host_call.gprs[1], 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}


