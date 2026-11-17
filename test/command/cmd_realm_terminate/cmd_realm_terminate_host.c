/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_rmi_sro.h"
#include "rmi_realm_terminate_data.h"
#include "command_common_host.h"

static struct argument_store {
    val_host_realm_ts realm;
    uint64_t rd_valid;
    uint64_t rd_undelegated;
} c_args;

struct arguments {
    uint64_t rd;
};

static uint64_t valid_input_args_prep_sequence(void)
{
    val_memset(&c_args, 0, sizeof(c_args));
    c_args.realm.s2sz = 40;
    c_args.realm.hash_algo = RMI_HASH_SHA_256;
    c_args.realm.s2_starting_level = 0;
    c_args.realm.num_s2_sl_rtts = 1;
    c_args.realm.vmid = 0;

    if (val_host_realm_create_common(&c_args.realm) != VAL_SUCCESS)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.rd_valid = c_args.realm.rd;

    c_args.rd_undelegated = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (c_args.rd_undelegated == 0)
        return VAL_TEST_PREP_SEQ_FAILED;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            break;

        case RD_STATE_UNDELEGATED:
            args->rd = c_args.rd_undelegated;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_realm_terminate_host(void)
{
    uint64_t i, expected_ret;
    val_smc_param_ts cmd_ret;
    struct arguments args;

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
            i + 1, test_data[i].msg, test_data[i].label);

        if (intent_to_seq(&test_data[i], &args))
        {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        cmd_ret = val_host_rmi_realm_terminate_smc(args.rd);
        expected_ret = PACK_CODE(test_data[i].status, test_data[i].index);
        if (cmd_ret.x0 != expected_ret)
        {
            LOG(ERROR, "RMI_REALM_TERMINATE returned 0x%x, expected 0x%x\n",
                cmd_ret.x0, expected_ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);
    if (val_host_rmi_realm_terminate(c_args.rd_valid) != RMI_SUCCESS)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    if (val_host_rmi_realm_destroy(c_args.rd_valid) != RMI_SUCCESS)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
