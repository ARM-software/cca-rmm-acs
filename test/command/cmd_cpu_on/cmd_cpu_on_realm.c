/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_realm_framework.h"
#include "psci_cpu_on_data.h"

#define MPIDR_RUNNABLE                1
#define MPIDR_NOT_RUNNABLE            2
#define MPIDR_NOT_IN_USE              3

#define CONTEXT_ID 0x5555

#define IPA_WIDTH 40
#define IPA_ADDR_UNPROTECTED (1UL << (IPA_WIDTH - 1))

static struct argument_store {
    uint64_t target_cpu_valid;
    uint64_t entry_point_address_valid;
    uint32_t context_id_valid;
} c_args;

struct arguments {
    uint64_t target_cpu;
    uint64_t entry_point_address;
    uint32_t context_id;
};

static void valid_argument_prep_sequence(void)
{
    c_args.target_cpu_valid = MPIDR_NOT_RUNNABLE;
    c_args.entry_point_address_valid = val_realm_get_secondary_cpu_entry();
    c_args.context_id_valid = CONTEXT_ID;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case ENTRY_ADDR_UNPROTECTED:
            args->target_cpu = c_args.target_cpu_valid;
            args->entry_point_address =  IPA_ADDR_UNPROTECTED;
            args->context_id = c_args.context_id_valid;
            break;

        case MPIDR_NOT_USED:
            args->target_cpu = MPIDR_NOT_IN_USE;
            args->entry_point_address =  c_args.entry_point_address_valid;
            args->context_id = c_args.context_id_valid;
            break;

        case REC_RUNNABLE:
            args->target_cpu = MPIDR_RUNNABLE;
            args->entry_point_address =  c_args.entry_point_address_valid;
            args->context_id = c_args.context_id_valid;
            break;

        default:
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_cpu_on_realm(void)
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

        ret = val_psci_cpu_on(args.target_cpu, args.entry_point_address, args.context_id);

        if (ret != test_data[i].status)
        {
            LOG(ERROR, "\n\tUnexpected Command Return Status\n ret status : 0x%x \n", ret, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }
    }


    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);

    /* Executing CPU on to a NOT_RUNNABLE REC should return PSCI_SUCCESS */
    ret = val_psci_cpu_on(c_args.target_cpu_valid, c_args.entry_point_address_valid,
                                                            c_args.context_id_valid);
    if (ret != PSCI_E_SUCCESS)
    {
        LOG(ERROR, "\n\tUnexpected Command Return Status\n ret status : 0x%x \n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    /* Subsequent execution of CPU_ON should fail with PSCI_ALREADY_ON */
    ret = val_psci_cpu_on(c_args.target_cpu_valid, c_args.entry_point_address_valid,
                                                            c_args.context_id_valid);
    if (ret != PSCI_E_ALREADY_ON)
    {
        LOG(ERROR, "\n\tUnexpected Command Return Status\n ret status : 0x%x \n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}

