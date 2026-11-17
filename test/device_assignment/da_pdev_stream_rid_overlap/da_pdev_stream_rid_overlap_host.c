/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "da_pdev_stream_invariant_host.h"
#include "command_common_host.h"
#include "val_host_pcie.h"
#include "val_host_rmi.h"

void da_pdev_stream_rid_overlap_host(void)
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

    if (PCIE_EXTRACT_BDF_SEG(context.endpoint[0].bdf) !=
        PCIE_EXTRACT_BDF_SEG(context.endpoint[1].bdf))
    {
        LOG(TEST, "Endpoints are in different PCIe segments\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    if (g_pdev_ready_prep_sequence(&context.endpoint[1]) == VAL_TEST_PREP_SEQ_FAILED)
    {
        LOG(ERROR, "Endpoint PDEV preparation failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto cleanup;
    }

    if (g_root_pdev_ready_prep_sequence(&context.stream, context.root_bdf) ==
        VAL_TEST_PREP_SEQ_FAILED)
    {
        LOG(ERROR, "Root Port PDEV preparation failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto cleanup;
    }

    context.params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (context.params == NULL)
    {
        LOG(ERROR, "PDEV stream parameter allocation failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto cleanup;
    }

    context.endpoint[0].rid_base =
        PCIE_CREATE_BDF_PACKED(context.endpoint[1].bdf);
    context.endpoint[0].rid_top = context.endpoint[0].rid_base + 1U;
    if (g_pdev_ready_prep_sequence(&context.endpoint[0]) ==
        VAL_TEST_PREP_SEQ_FAILED)
    {
        LOG(ERROR, "Could not prepare PDEV with overlapping RID range\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto cleanup;
    }

    LOG(TEST, "\n\tCheck overlapping endpoint RID range\n");
    da_pdev_stream_set_params(&context, 0U, 1U);
    ret = val_host_rmi_pdev_stream_connect((uint64_t)context.params);
    if (ret.x0 != RMI_ERROR_INPUT)
    {
        LOG(ERROR, "Overlapping RID range returned 0x%lx, expected RMI_ERROR_INPUT\n",
            ret.x0);
        if (ret.x0 == RMI_SUCCESS)
        {
            context.stream.stream_handle = ret.x1;
            if (da_pdev_stream_complete(&context, 0U,
                                        context.stream.stream_handle))
                LOG(ERROR, "Accepted stream could not be completed during cleanup\n");
        }
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto cleanup;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    if (da_pdev_stream_cleanup(&context))
    {
        LOG(ERROR, "PDEV stream cleanup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
    }
}
