/*
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_mec_set_private_data.h"

#define MECID_SHARED 0x1
#define MECID_PRIVATE 0x2
#define MECID_PRIVATE_UNASSIGNED 0x3

static struct argument_store {
    uint64_t mecid_valid;
} c_args;

struct arguments {
    uint64_t mecid;
};

static uint64_t g_mec_members_non_zero_prep_sequence(void)
{
    val_host_realm_ts realm1;

    val_memset(&realm1, 0, sizeof(realm1));

    val_host_realm_params(&realm1);

    realm1.mecid = MECID_PRIVATE;

    /* Populate realm-1 with Private MECID */
    if (val_host_realm_setup(&realm1, 1))
    {
        LOG(ERROR, "\tRealm-1 setup failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return MECID_PRIVATE;
}

static uint64_t g_mecid_bound_prep_sequence(void)
{
    uint64_t featreg1;

    val_host_rmi_features(1, &featreg1);

    return featreg1;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    val_smc_param_ts args;

    args = val_host_rmi_mec_set_shared(MECID_SHARED);
    if (args.x0)
    {
        LOG(ERROR, "\trmi_mec_set_shared failed %x\n", args.x0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    c_args.mecid_valid = MECID_SHARED;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case MECID_BOUND:
            args->mecid = g_mecid_bound_prep_sequence();
            break;

        case MEC_STATE_PRIVATE_UNASSIGNED:
            args->mecid = MECID_PRIVATE_UNASSIGNED;
            break;

        case MEC_MEMBERS_NON_ZERO:
            args->mecid = g_mec_members_non_zero_prep_sequence();
            if (args->mecid == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        default:
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_mec_set_private_host(void)
{
    uint64_t i;
    val_smc_param_ts cmd_ret;
    struct arguments args;
    uint64_t featreg1;

    val_host_rmi_features(1, &featreg1);
    if (!featreg1)
    {
        LOG(ERROR, "\tMEC feature not supported, skipping the test\n", 0, 0);
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto exit;
    }

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "\n\tCheck %d : ", i + 1, 0);
        LOG(TEST, test_data[i].msg, 0, 0);
        LOG(TEST, "; intent id : 0x%x \n", test_data[i].label, 0);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        cmd_ret = val_host_rmi_mec_set_private(args.mecid);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);
    cmd_ret = val_host_rmi_mec_set_private(c_args.mecid_valid);
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "\n\t Command failed. %x\n", cmd_ret.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
