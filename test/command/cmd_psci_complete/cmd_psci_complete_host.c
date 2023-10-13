/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
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

#define NUM_REALMS 2
#define VALID_REALM 0
#define INVALID_REALM 1

static val_host_realm_ts realm[NUM_REALMS];

static struct argument_store {
    uint64_t calling_rec_valid;
    uint64_t target_rec_valid;
} c_args;

struct arguments {
    uint64_t calling_rec;
    uint64_t target_rec;
};

static uint64_t g_calling_rec_no_request_prep_sequence(void)
{
    val_host_rec_params_ts rec_params;

    val_memset(&realm[INVALID_REALM], 0, sizeof(realm[INVALID_REALM]));

    realm[INVALID_REALM].s2sz = 40;
    realm[INVALID_REALM].hash_algo = RMI_HASH_SHA_256;
    realm[INVALID_REALM].s2_starting_level = 0;
    realm[INVALID_REALM].num_s2_sl_rtts = 1;
    realm[INVALID_REALM].vmid = 1;
    realm[INVALID_REALM].rec_count = 2;

    rec_params.pc = 0;
    rec_params.flags = RMI_RUNNABLE;


    if (val_host_realm_create_common(&realm[INVALID_REALM]))
    {
        LOG(ERROR, "\tRealm create failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Populate realm with two RECs*/
    if (val_host_rec_create_common(&realm[INVALID_REALM], &rec_params))
    {
        LOG(ERROR, "\tREC Create Failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm[INVALID_REALM].rec[0];
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
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm[VALID_REALM].rec[0], realm[VALID_REALM].run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Check that REC exit was due to PSCI_CPU_ON  */
    if (val_host_check_realm_exit_psci((val_host_rec_run_ts *)realm[VALID_REALM].run[0],
                                PSCI_CPU_ON_AARCH64))
    {
        LOG(ERROR, "\tSomething went wrong\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm[VALID_REALM].rec[0];
}

static uint64_t target_rec_valid_prep_sequence(void)
{
    return realm[VALID_REALM].rec[1];
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.calling_rec_valid = calling_rec_valid_prep_sequence();
    if (c_args.calling_rec_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.target_rec_valid = target_rec_valid_prep_sequence();

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case ALIAS:
            args->calling_rec = c_args.target_rec_valid;
            args->target_rec = c_args.target_rec_valid;
            break;

        case CALLING_UNALIGNED:
            args->calling_rec = g_unaligned_prep_sequence(c_args.calling_rec_valid);
            args->target_rec = c_args.target_rec_valid;
            break;

        case CALLING_DEV_MEM:
            args->calling_rec = g_dev_mem_prep_sequence();
            args->target_rec = c_args.target_rec_valid;
            break;

        case CALLING_OUT_OF_PERMITTED_PA:
            args->calling_rec = g_outside_of_permitted_pa_prep_sequence();
            args->target_rec = c_args.target_rec_valid;
            break;

        case CALLING_GRAN_STATE_UNDELEGATED:
            args->calling_rec = g_undelegated_prep_sequence();
            if (args->calling_rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->target_rec = c_args.target_rec_valid;
            break;

        case CALLING_GRAN_STATE_DELEGATED:
            args->calling_rec = g_delegated_prep_sequence();
            if (args->calling_rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->target_rec = c_args.target_rec_valid;
            break;

        case CALLING_GRAN_STATE_RD:
            args->calling_rec = realm[VALID_REALM].rd;
            args->target_rec = c_args.target_rec_valid;
            break;

        case CALLING_GRAN_STATE_RTT:
            args->calling_rec = realm[VALID_REALM].rtt_l0_addr;
            args->target_rec = c_args.target_rec_valid;
            break;

        case CALLING_GRAN_STATE_DATA:
            args->calling_rec = realm[VALID_REALM].image_pa_base;
            args->target_rec = c_args.target_rec_valid;
            break;

        case TARGET_UNALIGNED:
            args->calling_rec = c_args.calling_rec_valid;
            args->target_rec = g_unaligned_prep_sequence(c_args.target_rec_valid);
            break;

        case TARGET_DEV_MEM:
            args->calling_rec = c_args.calling_rec_valid;
            args->target_rec = g_dev_mem_prep_sequence();
            break;

        case TARGET_OUT_OF_PERMITTED_PA:
            args->calling_rec = c_args.calling_rec_valid;
            args->target_rec = g_outside_of_permitted_pa_prep_sequence();
            break;

        case TARGET_GRAN_STATE_UNDELEGATED:
            args->calling_rec = c_args.calling_rec_valid;
            args->target_rec = g_undelegated_prep_sequence();
            if (args->target_rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case TARGET_GRAN_STATE_DELEGATED:
            args->calling_rec = c_args.calling_rec_valid;
            args->target_rec = g_delegated_prep_sequence();
            if (args->target_rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case TARGET_GRAN_STATE_RD:
            args->calling_rec = c_args.calling_rec_valid;
            args->target_rec = realm[VALID_REALM].rd;
            break;

        case TARGET_GRAN_STATE_RTT:
            args->calling_rec = c_args.calling_rec_valid;
            args->target_rec = realm[VALID_REALM].rtt_l0_addr;
            break;

        case TARGET_GRAN_STATE_DATA:
            args->calling_rec = c_args.calling_rec_valid;
            args->target_rec = realm[VALID_REALM].image_pa_base;
            break;

        case NO_PSCI_REQUEST:
            args->calling_rec = g_calling_rec_no_request_prep_sequence();
            if (args->calling_rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->target_rec = realm[INVALID_REALM].rec[1];
            break;

        case TARGET_OTHER_OWNER:
            args->calling_rec = c_args.calling_rec_valid;
            args->target_rec = realm[INVALID_REALM].rec[1];
            break;

        case TARGET_OTHER_MPIDR:
            args->calling_rec = c_args.calling_rec_valid;
            args->target_rec = realm[VALID_REALM].rec[2];
            break;

        default:
            /* set status to failure */
            LOG(ERROR, "\t\nUnknown intent label encountered\n", 0, 0);
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
        LOG(TEST, "\n\tCheck %d : ", i + 1, 0);
        LOG(TEST, test_data[i].msg, 0, 0);
        LOG(TEST, "; intent id : 0x%x \n", test_data[i].label, 0);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        ret = val_host_rmi_psci_complete(args.calling_rec, args.target_rec);

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);

    /* Execute the command with valid input arguments */
    ret = val_host_rmi_psci_complete(c_args.calling_rec_valid, c_args.target_rec_valid);

    if (ret) {
        LOG(ERROR, "\t Command Failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
