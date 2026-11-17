/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "command_common_host.h"
#include "da_pdev_stream_invariant_host.h"
#include "val_host_alloc.h"
#include "val_host_pcie.h"
#include "val_host_rmi.h"

bool da_pdev_stream_ranges_are_ncoh(const val_host_pdev_ts *pdev)
{
    val_smc_param_ts ret;
    uint64_t base, top, i;

    if ((pdev->ncoh_num_addr_range == 0U) ||
        (pdev->ncoh_num_addr_range > MAX_PDEV_ADDR))
        return false;

    for (i = 0U; i < pdev->ncoh_num_addr_range; i++)
    {
        base = pdev->ncoh_addr_range[i].base;
        top = pdev->ncoh_addr_range[i].top;
        if ((base >= top) || ((base | top) & (PAGE_SIZE - 1U)))
            return false;

        while (base < top)
        {
            ret = val_host_rmi_granule_tracking_get(base, top);
            if ((ret.x0 != RMI_SUCCESS) ||
                (ret.x1 != RMI_MEM_CATEGORY_DEV_NCOH) ||
                (ret.x3 <= base) || (ret.x3 > top))
                return false;
            base = ret.x3;
        }
    }

    return true;
}

static bool ranges_are_disjoint(const val_host_pdev_ts *first,
                                const val_host_pdev_ts *second)
{
    uint64_t i, j;

    for (i = 0U; i < first->ncoh_num_addr_range; i++)
        for (j = 0U; j < second->ncoh_num_addr_range; j++)
            if ((first->ncoh_addr_range[i].base <
                 second->ncoh_addr_range[j].top) &&
                (second->ncoh_addr_range[j].base <
                 first->ncoh_addr_range[i].top))
                return false;

    return true;
}

static uint32_t find_endpoint_pair(da_pdev_stream_context_ts *context)
{
    val_host_pdev_ts first, second;
    uint32_t outer_count, inner_count, first_root, second_root;

    outer_count = val_pcie_get_num_bdf();
    if (outer_count == VAL_ERROR)
    {
        LOG(ERROR, "PCIe device enumeration failed\n");
        return VAL_ERROR;
    }

    while (outer_count != 0U)
    {
        val_memset(&first, 0, sizeof(first));
        if (val_pcie_find_doe_capability(&outer_count, &first.bdf,
                                         &first.doe_cap_base))
            break;

        if ((val_pcie_device_port_type(first.bdf) != EP) ||
            val_pcie_get_rootport(first.bdf, &first_root))
            continue;

        val_host_get_addr_range(&first);
        if (!da_pdev_stream_ranges_are_ncoh(&first))
            continue;

        inner_count = val_pcie_get_num_bdf();
        if (inner_count == VAL_ERROR)
        {
            LOG(ERROR, "PCIe device enumeration failed for second endpoint\n");
            return VAL_ERROR;
        }

        while (inner_count != 0U)
        {
            val_memset(&second, 0, sizeof(second));
            if (val_pcie_find_doe_capability(&inner_count, &second.bdf,
                                             &second.doe_cap_base))
                break;
            if ((second.bdf == first.bdf) ||
                (val_pcie_device_port_type(second.bdf) != EP) ||
                val_pcie_get_rootport(second.bdf, &second_root) ||
                (second_root != first_root))
                continue;

            val_host_get_addr_range(&second);
            if (!da_pdev_stream_ranges_are_ncoh(&second) ||
                !ranges_are_disjoint(&first, &second))
                continue;

            context->endpoint[0] = first;
            context->endpoint[1] = second;
            context->root_bdf = first_root;
            return VAL_SUCCESS;
        }
    }

    LOG(TEST, "No eligible pair of NCOH endpoints is available\n");
    return VAL_SKIP_CHECK;
}

uint32_t da_pdev_stream_init(da_pdev_stream_context_ts *context)
{
    val_host_pdev_flags_ts flags;
    uint32_t result;

    val_memset(context, 0, sizeof(*context));
    result = find_endpoint_pair(context);
    if (result != VAL_SUCCESS)
        return result;

    val_memset(&flags, 0, sizeof(flags));
    val_host_set_pdev_flags(&flags);

    context->endpoint[0].pdev_flags = flags;
    context->endpoint[1].pdev_flags = flags;
    context->endpoint[0].root_id = (uint16_t)context->root_bdf;
    context->endpoint[1].root_id = (uint16_t)context->root_bdf;

    return VAL_SUCCESS;
}

void da_pdev_stream_set_params(da_pdev_stream_context_ts *context,
                               uint32_t endpoint_index, uint32_t ide_sid)
{
    val_host_pdev_ts *pdev = &context->endpoint[endpoint_index];
    uint64_t i;

    val_memset(context->params, 0, PAGE_SIZE);
    context->params->stream_type = RMI_PDEV_STREAM_NCOH;
    context->params->pdev_1 = pdev->pdev;
    context->params->pdev_2 = context->stream.rp_pdev_obj.pdev;
    context->params->ide_sid = ide_sid;
    context->params->num_addr_range = pdev->ncoh_num_addr_range;

    for (i = 0U; i < pdev->ncoh_num_addr_range; i++)
    {
        context->params->addr_range[i].base = pdev->ncoh_addr_range[i].base;
        context->params->addr_range[i].top = pdev->ncoh_addr_range[i].top;
    }
}

uint32_t da_pdev_stream_complete(da_pdev_stream_context_ts *context,
                                  uint32_t endpoint_index, uint64_t handle)
{
    val_smc_param_ts ret;

    if (g_pdev_stream_communicate_prep_sequence(
                &context->endpoint[endpoint_index], &context->stream))
    {
        LOG(ERROR, "PDEV stream communication failed for endpoint %u\n",
            endpoint_index);
        return VAL_ERROR;
    }

    ret = val_host_rmi_pdev_stream_complete(
                    context->endpoint[endpoint_index].pdev,
                    context->stream.rp_pdev_obj.pdev, handle);
    if (ret.x0 != RMI_SUCCESS)
        LOG(ERROR, "PDEV stream completion failed for endpoint %u, ret=0x%lx\n",
            endpoint_index, ret.x0);

    return (ret.x0 == RMI_SUCCESS) ? VAL_SUCCESS : VAL_ERROR;
}

uint32_t da_pdev_stream_disconnect_second(da_pdev_stream_context_ts *context)
{
    val_smc_param_ts ret;

    ret = val_host_rmi_pdev_stream_disconnect(
                    context->endpoint[1].pdev,
                    context->stream.rp_pdev_obj.pdev,
                    context->second_handle);
    if (ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "Second PDEV stream disconnect failed, ret=0x%lx\n", ret.x0);
        return VAL_ERROR;
    }
    if (da_pdev_stream_complete(context, 1U, context->second_handle))
        return VAL_ERROR;

    context->second_handle = 0U;
    return VAL_SUCCESS;
}

uint32_t da_pdev_stream_cleanup_endpoint(val_host_pdev_ts *pdev)
{
    val_smc_param_ts ret;
    uint32_t result = VAL_SUCCESS;

    if ((pdev->pdev != 0U) &&
        (pdev->pdev != VAL_TEST_PREP_SEQ_FAILED))
    {
        ret = val_host_rmi_pdev_get_state(pdev->pdev);
        if ((ret.x0 == RMI_SUCCESS) &&
            val_host_pdev_teardown(pdev, pdev->pdev))
        {
            LOG(ERROR, "Endpoint PDEV teardown failed\n");
            result = VAL_ERROR;
        }
        else if (val_host_rmi_granule_range_undelegate(
                         pdev->pdev, pdev->pdev + PAGE_SIZE))
        {
            LOG(ERROR, "Endpoint PDEV undelegation failed\n");
            result = VAL_ERROR;
        }
        else
            val_host_mem_free((void *)pdev->pdev);
    }

    if (pdev->dev_comm_data != NULL)
    {
        if (pdev->dev_comm_data->enter.req_addr != 0U)
            val_host_mem_free((void *)pdev->dev_comm_data->enter.req_addr);
        if (pdev->dev_comm_data->enter.rsp_addr != 0U)
            val_host_mem_free((void *)pdev->dev_comm_data->enter.rsp_addr);
        val_host_mem_free(pdev->dev_comm_data);
    }

    if (pdev->cert_chain != NULL)
        val_host_mem_free(pdev->cert_chain);
    if (pdev->vca != NULL)
        val_host_mem_free(pdev->vca);
    if (pdev->public_key != NULL)
        val_host_mem_free(pdev->public_key);
    if (pdev->public_key_metadata != NULL)
        val_host_mem_free(pdev->public_key_metadata);

    return result;
}

uint32_t da_pdev_stream_cleanup(da_pdev_stream_context_ts *context)
{
    uint32_t result = VAL_SUCCESS;

    if ((context->second_handle != 0U) &&
        da_pdev_stream_disconnect_second(context))
    {
        LOG(ERROR, "Second PDEV stream cleanup failed\n");
        result = VAL_ERROR;
    }

    if ((context->stream.rp_pdev_obj.pdev != 0U) &&
        g_pdev_stream_teardown_sequence(&context->endpoint[0], &context->stream))
    {
        LOG(ERROR, "Root Port stream teardown failed\n");
        result = VAL_ERROR;
    }

    if (da_pdev_stream_cleanup_endpoint(&context->endpoint[1]))
    {
        LOG(ERROR, "Second endpoint PDEV cleanup failed\n");
        result = VAL_ERROR;
    }

    if (da_pdev_stream_cleanup_endpoint(&context->endpoint[0]))
    {
        LOG(ERROR, "First endpoint PDEV cleanup failed\n");
        result = VAL_ERROR;
    }

    if (context->params != NULL)
        val_host_mem_free(context->params);

    return result;
}
