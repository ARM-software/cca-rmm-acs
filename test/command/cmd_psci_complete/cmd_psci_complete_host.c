/*
 * Copyright (c) 2023, 2025-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_psci_complete_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define MAX_GRANULES 256

#define NUM_REALMS 1
#define VALID_REALM 0

static val_host_realm_ts realm[NUM_REALMS];

static struct argument_store {
    uint64_t calling_rec_valid;
    uint64_t status_valid;
} c_args;

struct arguments {
    uint64_t calling_rec;
    uint64_t status;
};

static uint64_t status_valid_prep_sequence(void)
{
    return PSCI_E_SUCCESS;
}

static uint64_t calling_rec_valid_prep_sequence(void)
{
    uint64_t ret;

    val_memset(&realm[VALID_REALM], 0, sizeof(realm[VALID_REALM]));

    val_host_realm_params(&realm[VALID_REALM]);

    realm[VALID_REALM].rec_count = 3;

    /* Populate realm with two RECs*/
    if (val_host_realm_setup(&realm[VALID_REALM], 1))
    {
        LOG(ERROR, "Realm setup failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm[VALID_REALM].rec[0], realm[VALID_REALM].run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Check that REC exit was due to PSCI_CPU_ON. */
    if (val_host_check_realm_exit_psci((val_host_rec_run_ts *)realm[VALID_REALM].run[0],
                                PSCI_CPU_ON_AARCH64))
    {
        LOG(ERROR, "Something went wrong\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm[VALID_REALM].rec[0];
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.calling_rec_valid = calling_rec_valid_prep_sequence();
    if (c_args.calling_rec_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.status_valid = status_valid_prep_sequence();

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case CALLING_UNALIGNED:
            args->calling_rec = g_unaligned_prep_sequence(c_args.calling_rec_valid);
            args->status = c_args.status_valid;
            break;


        case CALLING_GRAN_STATE_UNDELEGATED:
            args->calling_rec = g_undelegated_prep_sequence();
            if (args->calling_rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->status = c_args.status_valid;
            break;

        case CALLING_GRAN_STATE_DELEGATED:
            args->calling_rec = g_delegated_prep_sequence();
            if (args->calling_rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->status = c_args.status_valid;
            break;

        case CALLING_GRAN_STATE_RD:
            args->calling_rec = realm[VALID_REALM].rd;
            args->status = c_args.status_valid;
            break;

        case CALLING_GRAN_STATE_RTT:
            args->calling_rec = realm[VALID_REALM].rtt_l0_addr;
            args->status = c_args.status_valid;
            break;

        case CALLING_GRAN_STATE_DATA:
            args->calling_rec = realm[VALID_REALM].image_pa_base;
            args->status = c_args.status_valid;
            break;

        case NO_PSCI_REQUEST:
            args->calling_rec = realm[VALID_REALM].rec[2];
            args->status = c_args.status_valid;
            break;

        case STATUS_NOT_PERMITTED:
            args->calling_rec = c_args.calling_rec_valid;
            args->status = PSCI_E_NOT_SUPPORTED;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_psci_complete_host(void)
{
    uint64_t ret;
    struct arguments args;
    uint64_t i;

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    /* Iterate over the input */
    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
              i + 1, test_data[i].msg, test_data[i].label);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        ret = val_host_rmi_psci_complete(args.calling_rec, args.status);

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "Test Failure!The ABI call returned: %xExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);

    /* Execute the command with valid input arguments */
    ret = val_host_rmi_psci_complete(c_args.calling_rec_valid, c_args.status_valid);

    if (ret) {
        LOG(ERROR, " Command Failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    ret = val_host_rmi_rec_enter(realm[VALID_REALM].rec[0], realm[VALID_REALM].run[0]);
    if (ret || val_host_check_realm_exit_host_call(
                    (val_host_rec_run_ts *)realm[VALID_REALM].run[0])) {
        LOG(ERROR, "Calling REC did not resume after PSCI completion\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
