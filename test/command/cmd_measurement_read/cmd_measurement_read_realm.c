/*
 * Copyright (c) 2023, 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_realm_framework.h"
#include "rsi_measurement_read_data.h"

static struct argument_store {
    uint64_t index_valid;
} c_args;

struct arguments {
    uint64_t index;
};

static void valid_argument_prep_sequence(void)
{
    c_args.index_valid = 0;
}

static uint64_t index_bound_prep_sequence(void)
{
    /* Read the measurement for index greater than 4 value */
    return 5;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case INDEX_BOUND:
            args->index = index_bound_prep_sequence();
            break;

        default:
            // set status to failure
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_measurement_read_realm(void)
{
    uint64_t ret;
    struct arguments args;
    val_smc_param_ts cmd_ret;
    uint64_t i;

    /* Prepare valid arguments */
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

        cmd_ret = val_realm_rsi_measurement_read(args.index);
        ret = cmd_ret.x0;

        if (ret != test_data[i].status)
        {
            LOG(ERROR, "Unexpected Command Return Status ret status : 0x%x \n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);

    cmd_ret = val_realm_rsi_measurement_read(c_args.index_valid);
    ret = cmd_ret.x0;
    if (ret)
    {
        LOG(ERROR, "Positive Observability failed. Ret = 0x%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}

