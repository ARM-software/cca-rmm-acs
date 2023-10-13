/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_realm_framework.h"
#include "rsi_measurement_extend_data.h"

static struct argument_store {
    uint64_t index_valid;
    uint64_t size_valid;
} c_args;

struct arguments {
    uint64_t index;
    uint64_t size;
};

static void valid_argument_prep_sequence(void)
{
    c_args.index_valid = 1;
    c_args.size_valid = 32;
}

static uint64_t index_lower_bound_prep_sequence(void)
{
    /* Extend the measurement for index less than 1 value */
    return 0;
}

static uint64_t index_upper_bound_prep_sequence(void)
{
    /* Extend the measurement for index greater than 4 value */
    return 5;
}

static uint64_t size_bound_prep_sequence(void)
{
    /* Extend the measurement for size greater than 64 value */
    return 65;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case INDEX_LOWER_BOUND:
            args->index = index_lower_bound_prep_sequence();
            args->size = c_args.size_valid;
            break;

        case INDEX_UPPER_BOUND:
            args->index = index_upper_bound_prep_sequence();
            args->size = c_args.size_valid;
            break;

        case SIZE_BOUND:
            args->index = c_args.index_valid;
            args->size = size_bound_prep_sequence();
            break;

        default:
            // set status to failure
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_measurement_extend_realm(void)
{
    uint64_t ret;
    struct arguments args;
    uint64_t i;

    /* Prepare valid arguments */
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

        ret = val_realm_rsi_measurement_extend(args.index,
                                               args.size,
                                               0x05a9bf223fedf80a,
                                               0x9d0da5f73f5c191a,
                                               0x665bf4a0a4a3e608,
                                               0xf2f9e7d5ff23959c,
                                               0, 0, 0, 0);

        if (ret != test_data[i].status)
        {
            LOG(ERROR, "\n\tUnexpected Command Return Status\n ret status : 0x%x \n", ret, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);

    ret = val_realm_rsi_measurement_extend(c_args.index_valid,
                                           c_args.size_valid,
                                           0x05a9bf223fedf80a,
                                           0x9d0da5f73f5c191a,
                                           0x665bf4a0a4a3e608,
                                           0xf2f9e7d5ff23959c,
                                           0, 0, 0, 0);
    if (ret)
    {
        LOG(ERROR, "\n\tPositive Observability failed. Ret = 0x%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}

