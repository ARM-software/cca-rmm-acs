/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
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

static struct argument_store {
    uint64_t target_affinity_valid;
    uint32_t lowest_affinity_level_valid;
} c_args;

struct arguments {
    uint64_t target_affinity;
    uint32_t lowest_affinity_level;
};

static void valid_argument_prep_sequence(void)
{
    c_args.target_affinity_valid = MPIDR_RUNNABLE;
    c_args.lowest_affinity_level_valid = LOWEST_AFFINITY_LEVEL_VALID;
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
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
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

        ret = val_psci_affinity_info(args.target_affinity, args.lowest_affinity_level);

        if (ret != test_data[i].status)
        {
            LOG(ERROR, "\n\tUnexpected Command Return Status\n ret status : 0x%x \n", ret, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);

    /* Querying Affinity info for a runnable REC should return PSCI_SUCCESS */
    ret = val_psci_affinity_info(c_args.target_affinity_valid, c_args.lowest_affinity_level_valid);
    if (ret != PSCI_E_SUCCESS)
    {
        LOG(ERROR, "\n\tUnexpected Command Return Status\n ret status : 0x%x \n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    /* Querying Affinity info for a not runnable rec should return PSCI_OFF */
    ret = val_psci_affinity_info(MPIDR_NOT_RUNNABLE, c_args.lowest_affinity_level_valid);
    if (ret != PSCI_E_OFF)
    {
        LOG(ERROR, "\n\tUnexpected Command Return Status\n ret status : 0x%x \n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}

