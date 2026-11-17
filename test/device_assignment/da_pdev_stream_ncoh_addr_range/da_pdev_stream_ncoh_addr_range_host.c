/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_pcie.h"
#include "command_common_host.h"
#include "da_pdev_stream_invariant_host.h"

static uint64_t find_endpoint(val_host_pdev_ts *pdev)
{
    uint32_t num_bdf, rp_bdf;

    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR)
        return VAL_ERROR;

    while (num_bdf != 0U)
    {
        if (val_pcie_find_doe_capability(&num_bdf, &pdev->bdf,
                                         &pdev->doe_cap_base))
            break;

        if ((val_pcie_device_port_type(pdev->bdf) != EP) ||
            val_pcie_get_rootport(pdev->bdf, &rp_bdf))
            continue;
        pdev->root_id = (uint16_t)rp_bdf;

        val_host_get_addr_range(pdev);
        if ((pdev->ncoh_num_addr_range < MAX_PDEV_ADDR) &&
            da_pdev_stream_ranges_are_ncoh(pdev))
            return VAL_SUCCESS;
    }

    return VAL_SKIP_CHECK;
}

static uint64_t check_ncoh_stream(void)
{
    da_pdev_stream_context_ts context;
    val_host_rmi_pdev_stream_params_ts *params;
    val_host_pdev_stream_ts *stream = &context.stream;
    val_host_pdev_ts *pdev = &context.endpoint[0];
    val_host_pdev_flags_ts flags;
    val_smc_param_ts ret;
    void *wrong_range = NULL;
    uint64_t result, i;

    val_memset(&context, 0, sizeof(context));
    val_memset(&flags, 0, sizeof(flags));
    result = find_endpoint(pdev);
    if (result != VAL_SUCCESS)
    {
        if (result != VAL_SKIP_CHECK)
        {
            LOG(ERROR, "PDEV stream endpoint discovery failed\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        }
        return result;
    }

    val_host_set_pdev_flags(&flags);
    pdev->pdev_flags = flags;

    result = g_pdev_ready_prep_sequence(pdev);
    if (result == VAL_TEST_PREP_SEQ_FAILED)
    {
        LOG(ERROR, "Endpoint PDEV preparation failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        result = VAL_ERROR;
        goto cleanup;
    }

    result = g_root_pdev_ready_prep_sequence(stream, pdev->root_id);
    if (result == VAL_TEST_PREP_SEQ_FAILED)
    {
        LOG(ERROR, "Root Port PDEV preparation failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        result = VAL_ERROR;
        goto cleanup;
    }

    wrong_range = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    context.params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((wrong_range == NULL) || (context.params == NULL))
    {
        LOG(ERROR, "PDEV stream test buffer allocation failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        result = VAL_ERROR;
        goto cleanup;
    }

    ret = val_host_rmi_granule_tracking_get((uint64_t)wrong_range,
                                (uint64_t)wrong_range + PAGE_SIZE);
    if ((ret.x0 != RMI_SUCCESS) ||
        (ret.x1 != RMI_MEM_CATEGORY_CONVENTIONAL) ||
        (ret.x3 < (uint64_t)wrong_range + PAGE_SIZE))
    {
        LOG(ERROR, "Conventional-memory tracking check failed, ret=0x%lx, "
            "category=0x%lx, top=0x%lx\n", ret.x0, ret.x1, ret.x3);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        result = VAL_ERROR;
        goto cleanup;
    }

    da_pdev_stream_set_params(&context, 0U, 0U);
    params = context.params;
    i = params->num_addr_range;

    /* One conventional-memory entry makes the otherwise NCOH list invalid. */
    params->addr_range[i].base = (uint64_t)wrong_range;
    params->addr_range[i].top = (uint64_t)wrong_range + PAGE_SIZE;
    params->num_addr_range++;
    LOG(TEST, "\n\tCheck non-coherent stream address range category\n");
    ret = val_host_rmi_pdev_stream_connect((uint64_t)params);
    if (ret.x0 != RMI_ERROR_INPUT)
    {
        LOG(ERROR, "NCOH range check returned 0x%x, expected RMI_ERROR_INPUT\n",
            ret.x0);
        if (ret.x0 == RMI_SUCCESS)
        {
            stream->stream_handle = ret.x1;
            (void)da_pdev_stream_complete(&context, 0U,
                                          stream->stream_handle);
        }
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        result = VAL_ERROR;
        goto cleanup;
    }

    /* Verify the same PDEV pair accepts the original NCOH range list. */
    params->num_addr_range--;
    val_memset(&params->addr_range[params->num_addr_range], 0,
               sizeof(params->addr_range[0]));
    LOG(TEST, "\n\tVerify valid non-coherent stream\n");
    ret = val_host_rmi_pdev_stream_connect((uint64_t)params);
    if (ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "Valid NCOH stream connect failed, ret=0x%x\n", ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        result = VAL_ERROR;
        goto cleanup;
    }

    stream->stream_handle = ret.x1;
    result = da_pdev_stream_complete(&context, 0U, stream->stream_handle);
    if (result != VAL_SUCCESS)
    {
        LOG(ERROR, "Valid NCOH stream completion failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
    }

cleanup:
    if (g_pdev_stream_teardown_sequence(pdev, stream))
    {
        LOG(ERROR, "PDEV stream teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        result = VAL_ERROR;
    }
    if (da_pdev_stream_cleanup_endpoint(pdev))
    {
        LOG(ERROR, "Endpoint PDEV cleanup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        result = VAL_ERROR;
    }
    if (context.params != NULL)
        val_host_mem_free(context.params);
    if (wrong_range != NULL)
        val_host_mem_free(wrong_range);

    return result;
}

void da_pdev_stream_ncoh_addr_range_host(void)
{
    uint64_t result;

    if (!val_host_rmm_supports_da())
    {
        LOG(ERROR, "DA feature not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    result = check_ncoh_stream();
    if (result == VAL_SKIP_CHECK)
    {
        LOG(TEST, "No suitable NCOH endpoint is available\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    if (result != VAL_SUCCESS)
        return;

    val_set_status(RESULT_PASS(VAL_SUCCESS));
}
