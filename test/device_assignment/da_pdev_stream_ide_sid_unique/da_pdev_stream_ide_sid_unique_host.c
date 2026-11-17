/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "da_pdev_stream_invariant_host.h"
#include "command_common_host.h"
#include "val_host_rmi.h"

void da_pdev_stream_ide_sid_unique_host(void)
{
    da_pdev_stream_context_ts context;
    val_smc_param_ts ret;
    uint32_t result;

    if (!val_host_rmm_supports_da())
    {
        LOG(ERROR, "DA feature not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    result = da_pdev_stream_init(&context);
    if (result == VAL_SKIP_CHECK)
    {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }
    if (result != VAL_SUCCESS)
    {
        LOG(ERROR, "PDEV stream endpoint discovery failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    if (g_pdev_ready_prep_sequence(&context.endpoint[0]) ==
        VAL_TEST_PREP_SEQ_FAILED)
    {
        LOG(ERROR, "First endpoint PDEV preparation failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto cleanup;
    }

    if (g_pdev_ready_prep_sequence(&context.endpoint[1]) ==
        VAL_TEST_PREP_SEQ_FAILED)
    {
        LOG(ERROR, "Second endpoint PDEV preparation failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto cleanup;
    }

    if (g_root_pdev_ready_prep_sequence(&context.stream, context.root_bdf) ==
        VAL_TEST_PREP_SEQ_FAILED)
    {
        LOG(ERROR, "Root Port PDEV preparation failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto cleanup;
    }

    context.params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (context.params == NULL)
    {
        LOG(ERROR, "PDEV stream parameter allocation failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto cleanup;
    }

    da_pdev_stream_set_params(&context, 0U, 0U);
    ret = val_host_rmi_pdev_stream_connect((uint64_t)context.params);
    if (ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "First NCOH stream connect failed, ret=0x%lx\n", ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto cleanup;
    }
    context.stream.stream_handle = ret.x1;
    if (da_pdev_stream_complete(&context, 0U, context.stream.stream_handle))
    {
        LOG(ERROR, "First NCOH stream completion failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto cleanup;
    }

    da_pdev_stream_set_params(&context, 1U, 1U);
    ret = val_host_rmi_pdev_stream_connect((uint64_t)context.params);
    if (ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "Second valid NCOH stream connect failed, ret=0x%lx\n",
            ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto cleanup;
    }
    context.second_handle = ret.x1;
    if (da_pdev_stream_complete(&context, 1U, context.second_handle))
    {
        LOG(ERROR, "Second valid NCOH stream completion failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto cleanup;
    }
    if (da_pdev_stream_disconnect_second(&context))
    {
        LOG(ERROR, "Second valid NCOH stream disconnect failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto cleanup;
    }

    LOG(TEST, "\n\tCheck duplicate IDE stream identifier\n");
    da_pdev_stream_set_params(&context, 1U, 0U);
    ret = val_host_rmi_pdev_stream_connect((uint64_t)context.params);
    if (ret.x0 != RMI_ERROR_INPUT)
    {
        LOG(ERROR, "Duplicate IDE SID returned 0x%lx, expected RMI_ERROR_INPUT\n",
            ret.x0);
        if (ret.x0 == RMI_SUCCESS)
        {
            context.second_handle = ret.x1;
            if (da_pdev_stream_complete(&context, 1U, context.second_handle))
                LOG(ERROR, "Accepted stream could not be completed during cleanup\n");
        }
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto cleanup;
    }

    da_pdev_stream_set_params(&context, 1U, 1U);
    ret = val_host_rmi_pdev_stream_connect((uint64_t)context.params);
    if (ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "Valid stream retry failed after rejection, ret=0x%lx\n",
            ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto cleanup;
    }
    context.second_handle = ret.x1;
    if (da_pdev_stream_complete(&context, 1U, context.second_handle))
    {
        LOG(ERROR, "Valid NCOH stream retry completion failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto cleanup;
    }
    if (da_pdev_stream_disconnect_second(&context))
    {
        LOG(ERROR, "Valid NCOH stream retry disconnect failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(14)));
        goto cleanup;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    if (da_pdev_stream_cleanup(&context))
    {
        LOG(ERROR, "PDEV stream cleanup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(15)));
    }
}
