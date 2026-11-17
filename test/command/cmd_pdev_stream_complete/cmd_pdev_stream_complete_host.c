/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_pdev_stream_complete_data.h"
#include "command_common_host.h"

static struct argument_store {
    val_host_pdev_ts pdev;
    val_host_pdev_stream_ts pdev_stream;
} c_args;

struct arguments {
    uint64_t pdev_1;
    uint64_t pdev_2;
    uint64_t stream_handle;
};

static uint64_t valid_input_args_prep_sequence(void)
{
    return g_pdev_stream_connected_prep_sequence(
                                  &c_args.pdev,
                                  &c_args.pdev_stream,
                                  RMI_PDEV_STREAM_NCOH);
}

static uint64_t intent_to_seq(struct stimulus *test, struct arguments *args)
{
    args->pdev_1 = c_args.pdev.pdev;
    args->pdev_2 = c_args.pdev_stream.rp_pdev_obj.pdev;
    args->stream_handle = c_args.pdev_stream.stream_handle;

    switch (test->label)
    {
        case STREAM_INVALID:
            args->stream_handle++;
            break;

        case PDEV_1_UNALIGNED:
            args->pdev_1 = g_unaligned_prep_sequence(args->pdev_1);
            break;


        case PDEV_1_GRAN_STATE_UNDELEGATED:
            args->pdev_1 = g_undelegated_prep_sequence();
            if (args->pdev_1 == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case PDEV_2_UNALIGNED:
            args->pdev_2 = g_unaligned_prep_sequence(args->pdev_2);
            break;


        case PDEV_2_GRAN_STATE_UNDELEGATED:
            args->pdev_2 = g_undelegated_prep_sequence();
            if (args->pdev_2 == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_pdev_stream_complete_host(void)
{
    val_smc_param_ts cmd_ret;
    struct arguments args;
    uint64_t ret, i;

    if (!val_host_rmm_supports_da())
    {
        LOG(ALWAYS, "DA feature not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto exit;
    }

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_device;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "\n\tCheck %d : ", i + 1);
        LOG(TEST, test_data[i].msg);
        LOG(TEST, "; intent id : 0x%x \n", test_data[i].label);

        if (intent_to_seq(&test_data[i], &args))
        {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto destroy_device;
        }

        cmd_ret = val_host_rmi_pdev_stream_complete(
                                  args.pdev_1,
                                  args.pdev_2,
                                  args.stream_handle);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status,
                                    test_data[i].index))
        {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n"
                       "\tExpected: %x\n", cmd_ret.x0,
                PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_device;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n");
    cmd_ret = val_host_rmi_pdev_stream_disconnect(
                                  c_args.pdev.pdev,
                                  c_args.pdev_stream.rp_pdev_obj.pdev,
                                  c_args.pdev_stream.stream_handle);
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_PDEV_STREAM_DISCONNECT failed, ret=0x%x\n",
            cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    ret = g_pdev_stream_communicate_prep_sequence(
                                  &c_args.pdev,
                                  &c_args.pdev_stream);
    if (ret)
    {
        LOG(ERROR, "PDEV stream communication failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_device;
    }

    cmd_ret = val_host_rmi_pdev_stream_complete(
                                  c_args.pdev.pdev,
                                  c_args.pdev_stream.rp_pdev_obj.pdev,
                                  c_args.pdev_stream.stream_handle);
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_PDEV_STREAM_COMPLETE failed, ret=0x%x\n",
            cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_device;
    }
    c_args.pdev_stream.stream_handle = 0;

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    if (g_pdev_stream_teardown_sequence(&c_args.pdev,
                                        &c_args.pdev_stream))
    {
        LOG(ERROR, "PDEV stream teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
    }

    if (c_args.pdev.pdev &&
        (c_args.pdev.pdev != VAL_TEST_PREP_SEQ_FAILED) &&
        val_host_pdev_teardown(&c_args.pdev, c_args.pdev.pdev))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
    }

exit:
    return;
}
