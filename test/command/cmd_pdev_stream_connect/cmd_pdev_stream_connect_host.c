/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_pdev_stream_connect_data.h"
#include "command_common_host.h"
#include "val_host_pcie.h"

static struct argument_store {
    uint64_t featreg2;
    val_host_pdev_ts pdev;
    val_host_pdev_stream_ts pdev_stream;
    val_host_pdev_ts state_pdev;
    val_host_pdev_stream_ts state_pdev_stream;
    uint32_t state_stream_connecting;
    uint64_t params_ptr_valid;
    uint64_t params_ptr_allocated;
} c_args;

struct arguments {
    uint64_t params_ptr;
    bool uses_valid_pdev_pair;
};

typedef enum {
    PARAMS_NUM_ADDR_RANGE_INVALID = 0x0,
    PARAMS_ADDR_BASE_UNALIGNED,
    PARAMS_ADDR_TOP_UNALIGNED,
    PARAMS_ADDR_RANGE_OUTSIDE,
    PARAMS_ADDR_RANGE_OVERLAP,
    PARAMS_NON_TEE_STREAM,
    PARAMS_PDEV_1_UNALIGNED,
    PARAMS_PDEV_1_GRAN_STATE_UNDELEGATED,
    PARAMS_PDEV_1_INVALID_CATEGORY,
    PARAMS_PDEV_1_STATE_NEW,
    PARAMS_PDEV_2_UNALIGNED,
    PARAMS_PDEV_2_GRAN_STATE_UNDELEGATED,
    PARAMS_PDEV_2_INVALID_CATEGORY,
    PARAMS_PDEV_2_STATE_NEW,
    PARAMS_STREAM_NOT_DISCONNECTED,
    PARAMS_STREAM_TYPE_EXISTS
} params_prep_seq_type;

static uint64_t root_pdev_new_prep_sequence(
                            val_host_pdev_stream_ts *pdev_stream,
                            uint32_t rp_bdf)
{
    val_host_rmi_pdev_params_ts *params;
    val_host_pdev_flags_ts flags;
    val_host_pdev_ts *rp_pdev;
    uint64_t req_addr, resp_addr;
    uint64_t ret;

    rp_pdev = &pdev_stream->rp_pdev_obj;
    rp_pdev->pdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((rp_pdev->pdev == 0U) || (params == NULL))
        goto exit;

    ret = val_host_rmi_granule_range_delegate(rp_pdev->pdev,
                                               rp_pdev->pdev + PAGE_SIZE);
    if (ret)
        goto exit;
    pdev_stream->rp_pdev_delegated = 1;

    val_memset(params, 0, PAGE_SIZE);
    val_memset(&flags, 0, sizeof(flags));
    flags.category = RMI_PDEV_ROOT_PORT;
    val_memcpy(&params->flags, &flags, sizeof(flags));
    params->hb_base = val_pcie_get_ecam_base(rp_bdf);
    params->pdev_id = PCIE_CREATE_BDF_PACKED(rp_bdf);
    params->routing_id = PCIE_EXTRACT_BDF_SEG(rp_bdf);
    params->id_index = 0;
    params->rid_base = PCIE_CREATE_BDF_PACKED(rp_bdf);
    params->rid_top = params->rid_base + 1;

    ret = val_host_rmi_pdev_create(rp_pdev->pdev, (uint64_t)params);
    val_host_mem_free(params);
    params = NULL;
    if (ret)
        goto exit;
    pdev_stream->rp_pdev_created = 1;

    rp_pdev->bdf = rp_bdf;
    rp_pdev->pdev_flags = flags;
    rp_pdev->dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    req_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    resp_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((rp_pdev->dev_comm_data == NULL) ||
        (req_addr == 0U) || (resp_addr == 0U))
        goto exit;

    val_memset(rp_pdev->dev_comm_data, 0, PAGE_SIZE);
    rp_pdev->dev_comm_data->enter.req_addr = req_addr;
    rp_pdev->dev_comm_data->enter.rsp_addr = resp_addr;

    return rp_pdev->pdev;

exit:
    if (params != NULL)
        val_host_mem_free(params);

    return VAL_TEST_PREP_SEQ_FAILED;
}

static uint64_t complete_stream_operation(
                            val_host_pdev_ts *pdev,
                            val_host_pdev_stream_ts *pdev_stream,
                            uint64_t stream_handle)
{
    val_smc_param_ts cmd_ret;

    if (val_host_pdev_stream_communicate(
                            pdev,
                            &pdev_stream->rp_pdev_obj))
        return VAL_ERROR;

    cmd_ret = val_host_rmi_pdev_stream_complete(
                            pdev->pdev,
                            pdev_stream->rp_pdev_obj.pdev,
                            stream_handle);

    return (cmd_ret.x0 == RMI_SUCCESS) ? VAL_SUCCESS : VAL_ERROR;
}

static uint64_t disconnect_stream(
                            val_host_pdev_ts *pdev,
                            val_host_pdev_stream_ts *pdev_stream,
                            uint64_t stream_handle)
{
    val_smc_param_ts cmd_ret;

    cmd_ret = val_host_rmi_pdev_stream_disconnect(
                            pdev->pdev,
                            pdev_stream->rp_pdev_obj.pdev,
                            stream_handle);
    if (cmd_ret.x0 != RMI_SUCCESS)
        return VAL_ERROR;

    return complete_stream_operation(pdev, pdev_stream, stream_handle);
}

static uint64_t connected_stream_prep_sequence(
                            val_host_pdev_ts *pdev,
                            val_host_pdev_stream_ts *pdev_stream,
                            val_host_rmi_pdev_stream_params_ts *params)
{
    val_smc_param_ts cmd_ret;

    cmd_ret = val_host_rmi_pdev_stream_connect(
                                    (uint64_t)params);
    if (cmd_ret.x0 != RMI_SUCCESS)
        return VAL_ERROR;

    pdev_stream->stream_handle = cmd_ret.x1;

    return complete_stream_operation(pdev, pdev_stream,
                                     pdev_stream->stream_handle);
}

static uint64_t state_pdev_new_prep_sequence(void)
{
    val_host_pdev_flags_ts pdev_flags;
    uint32_t num_bdf, rp_bdf;
    uint64_t ret;

    val_memset(&c_args.state_pdev, 0,
               sizeof(c_args.state_pdev));
    val_memset(&c_args.state_pdev_stream, 0,
               sizeof(c_args.state_pdev_stream));
    c_args.state_stream_connecting = 0;

    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR)
        return VAL_ERROR;

    while (num_bdf != 0U)
    {
        if (val_pcie_find_doe_capability(
                            &num_bdf,
                            &c_args.state_pdev.bdf,
                            &c_args.state_pdev.doe_cap_base))
            break;

        if ((c_args.state_pdev.bdf == c_args.pdev.bdf) ||
            (val_pcie_device_port_type(
                            c_args.state_pdev.bdf) != EP) ||
            val_pcie_get_rootport(c_args.state_pdev.bdf,
                                  &rp_bdf) ||
            (rp_bdf == c_args.pdev.root_id))
            continue;

        val_memset(&pdev_flags, 0, sizeof(pdev_flags));
        val_host_set_pdev_flags(&pdev_flags);
        c_args.state_pdev.pdev_flags = pdev_flags;
        c_args.state_pdev.root_id = (uint16_t)rp_bdf;

        ret = g_pdev_new_prep_sequence(&c_args.state_pdev);
        if (ret == VAL_TEST_PREP_SEQ_FAILED)
            return VAL_ERROR;

        return VAL_SUCCESS;
    }

    LOG(ERROR, "No second DOE endpoint on a different root port\n");
    return VAL_ERROR;
}

static uint64_t pdev_stream_state_prep_sequence(
                            enum test_intent label,
                            val_host_rmi_pdev_stream_params_ts *params)
{
    val_smc_param_ts cmd_ret;
    uint64_t ret, i;

    ret = state_pdev_new_prep_sequence();
    if (ret != VAL_SUCCESS)
        return ret;

    if ((label != PDEV_1_STATE_NEW) &&
        (g_pdev_ready_prep_sequence(&c_args.state_pdev) ==
         VAL_TEST_PREP_SEQ_FAILED))
        return VAL_ERROR;

    if (root_pdev_new_prep_sequence(
                            &c_args.state_pdev_stream,
                            c_args.state_pdev.root_id) ==
        VAL_TEST_PREP_SEQ_FAILED)
        return VAL_ERROR;

    if ((label != PDEV_2_STATE_NEW) &&
        val_host_dev_communicate(
                    NULL,
                    &c_args.state_pdev_stream.rp_pdev_obj,
                    NULL, RMI_PDEV_READY))
        return VAL_ERROR;

    params->stream_type = RMI_PDEV_STREAM_NCOH;
    params->pdev_1 = c_args.state_pdev.pdev;
    params->pdev_2 =
                    c_args.state_pdev_stream.rp_pdev_obj.pdev;
    params->ide_sid = 0;

    if ((c_args.state_pdev.ncoh_num_addr_range >
         NCOH_ADDR_RANGE_NUM) ||
        (c_args.state_pdev.ncoh_num_addr_range > MAX_PDEV_ADDR))
        return VAL_ERROR;

    params->num_addr_range =
                    c_args.state_pdev.ncoh_num_addr_range;
    for (i = 0; i < params->num_addr_range; i++)
    {
        params->addr_range[i].base =
                    c_args.state_pdev.ncoh_addr_range[i].base;
        params->addr_range[i].top =
                    c_args.state_pdev.ncoh_addr_range[i].top;
    }

    if (label == STREAM_NOT_DISCONNECTED)
    {
        cmd_ret = val_host_rmi_pdev_stream_connect((uint64_t)params);
        if (cmd_ret.x0 != RMI_SUCCESS)
            return VAL_ERROR;

        c_args.state_pdev_stream.stream_handle = cmd_ret.x1;
        c_args.state_stream_connecting = 1;
    }
    else if (label == STREAM_TYPE_EXISTS)
    {
        if (connected_stream_prep_sequence(
                            &c_args.state_pdev,
                            &c_args.state_pdev_stream,
                            params))
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t params_valid_prep_sequence(void)
{
    val_host_rmi_pdev_stream_params_ts *params;
    uint64_t i;

    if ((c_args.pdev.ncoh_num_addr_range > NCOH_ADDR_RANGE_NUM) ||
        (c_args.pdev.ncoh_num_addr_range > MAX_PDEV_ADDR))
        return VAL_TEST_PREP_SEQ_FAILED;

    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (params == NULL)
        return VAL_TEST_PREP_SEQ_FAILED;

    val_memset(params, 0, PAGE_SIZE);
    params->stream_type = RMI_PDEV_STREAM_NCOH;
    params->pdev_1 = c_args.pdev.pdev;
    params->pdev_2 = c_args.pdev_stream.rp_pdev_obj.pdev;
    params->ide_sid = 0;
    params->num_addr_range = c_args.pdev.ncoh_num_addr_range;
    for (i = 0; i < params->num_addr_range; i++)
    {
        params->addr_range[i].base =
                            c_args.pdev.ncoh_addr_range[i].base;
        params->addr_range[i].top =
                            c_args.pdev.ncoh_addr_range[i].top;
    }

    return (uint64_t)params;
}

static uint64_t g_params_prep_sequence(params_prep_seq_type type)
{
    val_host_rmi_pdev_stream_params_ts *params;
    uint64_t ret, i;

    if ((c_args.pdev.ncoh_num_addr_range > NCOH_ADDR_RANGE_NUM) ||
        (c_args.pdev.ncoh_num_addr_range > MAX_PDEV_ADDR))
        return VAL_TEST_PREP_SEQ_FAILED;

    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (params == NULL)
        return VAL_TEST_PREP_SEQ_FAILED;

    val_memset(params, 0, PAGE_SIZE);
    params->stream_type = RMI_PDEV_STREAM_NCOH;
    params->pdev_1 = c_args.pdev.pdev;
    params->pdev_2 = c_args.pdev_stream.rp_pdev_obj.pdev;
    params->ide_sid = 0;
    params->num_addr_range = c_args.pdev.ncoh_num_addr_range;
    for (i = 0; i < params->num_addr_range; i++)
    {
        params->addr_range[i].base =
                            c_args.pdev.ncoh_addr_range[i].base;
        params->addr_range[i].top =
                            c_args.pdev.ncoh_addr_range[i].top;
    }

    switch (type)
    {
        case PARAMS_NUM_ADDR_RANGE_INVALID:
            params->num_addr_range = MAX_PDEV_ADDR + 1;
            break;

        case PARAMS_ADDR_BASE_UNALIGNED:
            if (params->num_addr_range == 0U)
                goto missing_addr_range;
            params->addr_range[0].base++;
            break;

        case PARAMS_ADDR_TOP_UNALIGNED:
            if (params->num_addr_range == 0U)
                goto missing_addr_range;
            params->addr_range[0].top--;
            break;

        case PARAMS_ADDR_RANGE_OUTSIDE:
            params->num_addr_range = 1U;
            params->addr_range[0].base =
                    g_outside_of_permitted_pa_prep_sequence();
            params->addr_range[0].top =
                    params->addr_range[0].base + PAGE_SIZE;
            break;

        case PARAMS_ADDR_RANGE_OVERLAP:
            if (params->num_addr_range == 0U)
                goto missing_addr_range;
            params->num_addr_range = 2U;
            params->addr_range[1] = params->addr_range[0];
            break;

        case PARAMS_NON_TEE_STREAM:
            params->stream_type = RMI_PDEV_STREAM_NON_TEE;
            break;

        case PARAMS_PDEV_1_UNALIGNED:
            params->pdev_1 =
                    g_unaligned_prep_sequence(params->pdev_1);
            break;

        case PARAMS_PDEV_1_GRAN_STATE_UNDELEGATED:
            params->pdev_1 = g_undelegated_prep_sequence();
            break;

        case PARAMS_PDEV_1_INVALID_CATEGORY:
            params->pdev_1 = c_args.pdev_stream.rp_pdev_obj.pdev;
            break;

        case PARAMS_PDEV_1_STATE_NEW:
            ret = pdev_stream_state_prep_sequence(PDEV_1_STATE_NEW,
                                                  params);
            goto state_prep_done;

        case PARAMS_PDEV_2_UNALIGNED:
            params->pdev_2 =
                    g_unaligned_prep_sequence(params->pdev_2);
            break;

        case PARAMS_PDEV_2_GRAN_STATE_UNDELEGATED:
            params->pdev_2 = g_undelegated_prep_sequence();
            break;

        case PARAMS_PDEV_2_INVALID_CATEGORY:
            params->pdev_2 = c_args.pdev.pdev;
            break;

        case PARAMS_PDEV_2_STATE_NEW:
            ret = pdev_stream_state_prep_sequence(PDEV_2_STATE_NEW,
                                                  params);
            goto state_prep_done;

        case PARAMS_STREAM_NOT_DISCONNECTED:
            ret = pdev_stream_state_prep_sequence(
                                    STREAM_NOT_DISCONNECTED, params);
            goto state_prep_done;

        case PARAMS_STREAM_TYPE_EXISTS:
            ret = pdev_stream_state_prep_sequence(
                                    STREAM_TYPE_EXISTS, params);
            goto state_prep_done;

        default:
            val_host_mem_free(params);
            return VAL_TEST_PREP_SEQ_FAILED;
    }

    c_args.params_ptr_allocated = (uint64_t)params;

    return (uint64_t)params;

state_prep_done:
    if (ret == VAL_SUCCESS)
    {
        c_args.params_ptr_allocated = (uint64_t)params;
        return (uint64_t)params;
    }

    val_host_mem_free(params);
    return VAL_TEST_PREP_SEQ_FAILED;

missing_addr_range:
    LOG(ERROR, "No NCOH address range for stream-connect check\n");
    val_host_mem_free(params);
    return VAL_TEST_PREP_SEQ_FAILED;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    val_host_pdev_flags_ts pdev_flags;
    uint32_t num_bdf, rp_bdf;

    val_memset(&c_args, 0, sizeof(c_args));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR)
        return VAL_TEST_PREP_SEQ_FAILED;

    if (val_pcie_find_doe_capability(&num_bdf, &c_args.pdev.bdf,
                                     &c_args.pdev.doe_cap_base))
        return VAL_TEST_PREP_SEQ_FAILED;

    if (val_pcie_get_rootport(c_args.pdev.bdf, &rp_bdf))
        return VAL_TEST_PREP_SEQ_FAILED;

    val_host_set_pdev_flags(&pdev_flags);
    c_args.pdev.pdev_flags = pdev_flags;
    c_args.pdev.root_id = (uint16_t)rp_bdf;
    c_args.pdev.pdev = g_pdev_ready_prep_sequence(&c_args.pdev);
    if (c_args.pdev.pdev == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    if (root_pdev_new_prep_sequence(&c_args.pdev_stream, rp_bdf) ==
        VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    if (val_host_dev_communicate(NULL,
                    &c_args.pdev_stream.rp_pdev_obj,
                    NULL, RMI_PDEV_READY))
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.params_ptr_valid = params_valid_prep_sequence();
    if (c_args.params_ptr_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    if (val_host_rmi_features(2, &c_args.featreg2))
        return VAL_TEST_PREP_SEQ_FAILED;

    return VAL_SUCCESS;
}

static uint64_t pdev_stream_state_cleanup(void)
{
    uint64_t pdev = c_args.state_pdev.pdev;
    uint64_t status = VAL_SUCCESS;

    if (c_args.state_stream_connecting &&
        complete_stream_operation(
                    &c_args.state_pdev,
                    &c_args.state_pdev_stream,
                    c_args.state_pdev_stream.stream_handle))
        status = VAL_ERROR;
    c_args.state_stream_connecting = 0;

    if (c_args.state_pdev_stream.rp_pdev_obj.pdev &&
        g_pdev_stream_teardown_sequence(
                    &c_args.state_pdev,
                    &c_args.state_pdev_stream))
        status = VAL_ERROR;

    if ((pdev != 0U) && (pdev != VAL_TEST_PREP_SEQ_FAILED))
    {
        if (val_host_pdev_teardown(&c_args.state_pdev, pdev))
        {
            status = VAL_ERROR;
        }
        else if (val_host_rmi_granule_range_undelegate(
                                            pdev, pdev + PAGE_SIZE))
        {
            status = VAL_ERROR;
        }
        else
        {
            val_host_mem_free((void *)pdev);
        }
    }

    val_memset(&c_args.state_pdev, 0,
               sizeof(c_args.state_pdev));
    val_memset(&c_args.state_pdev_stream, 0,
               sizeof(c_args.state_pdev_stream));

    return status;
}

static uint64_t intent_to_seq(struct stimulus *test, struct arguments *args)
{
    val_host_rmi_pdev_stream_params_ts *params;

    args->params_ptr = c_args.params_ptr_valid;
    args->uses_valid_pdev_pair = false;

    switch (test->label)
    {
        case PARAMS_UNALIGNED:
            args->params_ptr = g_unaligned_prep_sequence(
                                            c_args.params_ptr_valid);
            break;

        case PARAMS_PAS_REALM:
            args->params_ptr = g_delegated_prep_sequence();
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case PARAMS_PAS_SECURE:
            args->params_ptr = g_secure_prep_sequence();
            break;

        case PARAMS_INVALID:
            args->params_ptr = g_params_prep_sequence(
                            PARAMS_NUM_ADDR_RANGE_INVALID);
            break;

        case ADDR_BASE_UNALIGNED:
            args->params_ptr = g_params_prep_sequence(
                            PARAMS_ADDR_BASE_UNALIGNED);
            break;

        case ADDR_TOP_UNALIGNED:
            args->params_ptr = g_params_prep_sequence(
                            PARAMS_ADDR_TOP_UNALIGNED);
            break;

        case ADDR_RANGE_OUTSIDE:
            args->params_ptr = g_params_prep_sequence(
                            PARAMS_ADDR_RANGE_OUTSIDE);
            break;

        case ADDR_RANGE_OVERLAP:
            args->params_ptr = g_params_prep_sequence(
                            PARAMS_ADDR_RANGE_OVERLAP);
            break;

        case FEATURE_NON_TEE_STREAM:
            if (VAL_EXTRACT_BITS(c_args.featreg2,
                                 RMI_FEATURE_REGISTER_2_NON_TEE_STREAM,
                                 RMI_FEATURE_REGISTER_2_NON_TEE_STREAM) ==
                RMI_FEATURE_TRUE)
                return VAL_SKIP_CHECK;
            args->params_ptr = g_params_prep_sequence(
                            PARAMS_NON_TEE_STREAM);
            break;

        case PDEV_1_UNALIGNED:
            args->params_ptr = g_params_prep_sequence(
                            PARAMS_PDEV_1_UNALIGNED);
            break;

        case PDEV_1_GRAN_STATE_UNDELEGATED:
            args->params_ptr = g_params_prep_sequence(
                            PARAMS_PDEV_1_GRAN_STATE_UNDELEGATED);
            break;

        case PDEV_1_INVALID_CATEGORY:
            args->params_ptr = g_params_prep_sequence(
                            PARAMS_PDEV_1_INVALID_CATEGORY);
            break;

        case PDEV_1_STATE_NEW:
            args->params_ptr = g_params_prep_sequence(
                            PARAMS_PDEV_1_STATE_NEW);
            break;

        case PDEV_2_UNALIGNED:
            args->params_ptr = g_params_prep_sequence(
                            PARAMS_PDEV_2_UNALIGNED);
            break;

        case PDEV_2_GRAN_STATE_UNDELEGATED:
            args->params_ptr = g_params_prep_sequence(
                            PARAMS_PDEV_2_GRAN_STATE_UNDELEGATED);
            break;

        case PDEV_2_INVALID_CATEGORY:
            args->params_ptr = g_params_prep_sequence(
                            PARAMS_PDEV_2_INVALID_CATEGORY);
            break;

        case PDEV_2_STATE_NEW:
            args->params_ptr = g_params_prep_sequence(
                            PARAMS_PDEV_2_STATE_NEW);
            break;

        case STREAM_NOT_DISCONNECTED:
            args->params_ptr = g_params_prep_sequence(
                            PARAMS_STREAM_NOT_DISCONNECTED);
            break;

        case STREAM_TYPE_EXISTS:
            args->params_ptr = g_params_prep_sequence(
                            PARAMS_STREAM_TYPE_EXISTS);
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    if (args->params_ptr == VAL_SKIP_CHECK)
        return VAL_ERROR;

    if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_ERROR;

    if ((c_args.params_ptr_allocated != 0U) &&
        (args->params_ptr == c_args.params_ptr_allocated))
    {
        params = (val_host_rmi_pdev_stream_params_ts *)args->params_ptr;
        args->uses_valid_pdev_pair =
            (params->pdev_1 == c_args.pdev.pdev) &&
            (params->pdev_2 == c_args.pdev_stream.rp_pdev_obj.pdev);
    }

    return VAL_SUCCESS;
}

void cmd_pdev_stream_connect_host(void)
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

        ret = intent_to_seq(&test_data[i], &args);
        if (ret == VAL_SKIP_CHECK)
        {
            LOG(TEST, "\tFEAT_NON_TEE_STREAM is supported; check not applicable\n");
            continue;
        }
        if (ret)
        {
            (void)pdev_stream_state_cleanup();
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto destroy_device;
        }

        cmd_ret = val_host_rmi_pdev_stream_connect(args.params_ptr);
        if (c_args.params_ptr_allocated != 0U)
        {
            val_host_mem_free((void *)c_args.params_ptr_allocated);
            c_args.params_ptr_allocated = 0U;
        }

        if (cmd_ret.x0 != PACK_CODE(test_data[i].status,
                                    test_data[i].index))
        {
            if ((cmd_ret.x0 == RMI_SUCCESS) && args.uses_valid_pdev_pair)
            {
                ret = complete_stream_operation(
                                &c_args.pdev,
                                &c_args.pdev_stream,
                                cmd_ret.x1);
                if (ret == VAL_SUCCESS)
                    ret = disconnect_stream(
                                &c_args.pdev,
                                &c_args.pdev_stream,
                                cmd_ret.x1);
                if (ret != VAL_SUCCESS)
                    LOG(ERROR, "Unexpected stream cleanup failed\n");
            }

            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n"
                       "\tExpected: %x\n", cmd_ret.x0,
                PACK_CODE(test_data[i].status, test_data[i].index));
            (void)pdev_stream_state_cleanup();
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_device;
        }

        if (pdev_stream_state_cleanup())
        {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto destroy_device;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n");
    cmd_ret = val_host_rmi_pdev_stream_connect(
                                    c_args.params_ptr_valid);
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_PDEV_STREAM_CONNECT failed, ret=0x%x\n",
            cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_device;
    }
    c_args.pdev_stream.stream_handle = cmd_ret.x1;

    if (complete_stream_operation(
                            &c_args.pdev,
                            &c_args.pdev_stream,
                            c_args.pdev_stream.stream_handle))
    {
        LOG(ERROR, "PDEV stream completion failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_device;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    if (c_args.params_ptr_allocated != 0U)
    {
        val_host_mem_free((void *)c_args.params_ptr_allocated);
        c_args.params_ptr_allocated = 0U;
    }

    if (pdev_stream_state_cleanup())
    {
        LOG(ERROR, "PDEV stream state teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
    }

    if (c_args.pdev_stream.rp_pdev_obj.pdev &&
        val_host_pdev_stream_teardown(&c_args.pdev,
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

    if (c_args.params_ptr_valid != 0U)
        val_host_mem_free((void *)c_args.params_ptr_valid);

exit:
    return;
}
