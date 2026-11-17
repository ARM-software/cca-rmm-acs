/*
 * Copyright (c) 2023, 2025-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_realm_framework.h"
#include "rsi_measurement_extend_data.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"

#define MEASUREMENT_WORDS 8U

static const uint64_t measurement_data[MEASUREMENT_WORDS] = {
    0x05a9bf223fedf80a,
    0x9d0da5f73f5c191a,
    0x665bf4a0a4a3e608,
    0xf2f9e7d5ff23959c,
    0x0123456789abcdef,
    0xfedcba9876543210,
    0x1122334455667788,
    0x8877665544332211
};

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
    c_args.size_valid = sizeof(measurement_data);
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

static uint64_t extend_measurement(uint64_t index, uint64_t size)
{
    return val_realm_rsi_measurement_extend(index, size,
                                            measurement_data[0],
                                            measurement_data[1],
                                            measurement_data[2],
                                            measurement_data[3],
                                            measurement_data[4],
                                            measurement_data[5],
                                            measurement_data[6],
                                            measurement_data[7]);
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
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_measurement_extend_realm(void)
{
    uint64_t *hash_algorithm =
        (uint64_t *)(val_get_shared_region_base() + TEST_USE_OFFSET1);
    val_smc_param_ts measurement_before, measurement_after;
    uint64_t hash_input[MEASUREMENT_WORDS * 2U] = {0};
    uint64_t expected_measurement[MEASUREMENT_WORDS] = {0};
    int hash_ret;
    uint64_t ret;
    struct arguments args;
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

        ret = extend_measurement(args.index, args.size);

        if (ret != test_data[i].status)
        {
            LOG(ERROR, "Unexpected Command Return Status ret status : 0x%x \n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);

    measurement_before = val_realm_rsi_measurement_read(c_args.index_valid);
    if (measurement_before.x0 != RSI_SUCCESS) {
        LOG(ERROR, "Measurement read before extend failed. Ret = 0x%lx\n",
            measurement_before.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    val_memcpy(hash_input, &measurement_before.x1,
               sizeof(expected_measurement));
    val_memcpy(&hash_input[MEASUREMENT_WORDS], measurement_data,
               sizeof(measurement_data));

    if (*hash_algorithm == RSI_HASH_SHA_512)
        hash_ret = mbedtls_sha512((const unsigned char *)hash_input,
                                  sizeof(hash_input),
                                  (unsigned char *)expected_measurement, 0);
    else
        hash_ret = mbedtls_sha256((const unsigned char *)hash_input,
                                  sizeof(hash_input),
                                  (unsigned char *)expected_measurement, 0);

    if (hash_ret)
    {
        LOG(ERROR, "Failed to calculate expected REM value\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    ret = extend_measurement(c_args.index_valid, c_args.size_valid);
    if (ret)
    {
        LOG(ERROR, "Positive Observability failed. Ret = 0x%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    measurement_after = val_realm_rsi_measurement_read(c_args.index_valid);
    if (measurement_after.x0 != RSI_SUCCESS) {
        LOG(ERROR, "Measurement read after extend failed. Ret = 0x%lx\n",
            measurement_after.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    if (val_memcmp(&measurement_after.x1, expected_measurement,
                   sizeof(expected_measurement))) {
        LOG(ERROR, "Extended REM does not match RemExtend\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    val_realm_return_to_host();
}
