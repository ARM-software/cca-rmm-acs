/*
 * Copyright (c) 2023, 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_realm_framework.h"
#include "psci_affinity_info_data.h"

#define MPIDR_RUNNABLE                1
#define MPIDR_NOT_RUNNABLE            2
#define MPIDR_NOT_IN_USE              3

#define LOWEST_AFFINITY_LEVEL_VALID   0
#define LOWEST_AFFINITY_LEVEL_INVALID 1

#define CONTEXT_ID 0x5555

static struct argument_store {
    uint64_t target_affinity_valid;
    uint32_t lowest_affinity_level_valid;
} c_args;

struct arguments {
    uint64_t target_affinity;
    uint32_t lowest_affinity_level;
};

static uint64_t mpidr_runnable_prep_sequence(void)
{
    uint64_t ret;
    /* Power on REC[1] for execution */
    ret = val_psci_cpu_on(MPIDR_RUNNABLE, val_realm_get_secondary_cpu_entry(), CONTEXT_ID);
    if (ret)
    {
        LOG(ERROR, "PSCI CPU ON failed with ret status : 0x%x \n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return MPIDR_RUNNABLE;
}

static uint64_t valid_argument_prep_sequence(void)
{
    c_args.target_affinity_valid = mpidr_runnable_prep_sequence();
    if (c_args.target_affinity_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.lowest_affinity_level_valid = LOWEST_AFFINITY_LEVEL_VALID;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case INVALID_LOWEST_AFFINITY_LEVEL:
            args->target_affinity = c_args.target_affinity_valid;
            args->lowest_affinity_level = LOWEST_AFFINITY_LEVEL_INVALID;
            break;

        case MPIDR_NOT_USED:
            args->target_affinity = MPIDR_NOT_IN_USE;
            args->lowest_affinity_level = c_args.lowest_affinity_level_valid;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_affinity_info_realm(void)
{
    uint64_t ret;
    struct arguments args;
    uint64_t i;

    /* Prepare Valid arguments */
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

        ret = val_psci_affinity_info(args.target_affinity, args.lowest_affinity_level);

        if (ret != test_data[i].status)
        {
            LOG(ERROR, "Unexpected Command Return Status ret status : 0x%x \n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);

    /* Querying Affinity info for a runnable REC should return PSCI_SUCCESS */
    ret = val_psci_affinity_info(c_args.target_affinity_valid, c_args.lowest_affinity_level_valid);
    if (ret != PSCI_E_SUCCESS)
    {
        LOG(ERROR, "Unexpected Command Return Status ret status : 0x%x \n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    /* Querying Affinity info for a not runnable rec should return PSCI_OFF */
    ret = val_psci_affinity_info(MPIDR_NOT_RUNNABLE, c_args.lowest_affinity_level_valid);
    if (ret != PSCI_E_OFF)
    {
        LOG(ERROR, "Unexpected Command Return Status ret status : 0x%x \n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}

