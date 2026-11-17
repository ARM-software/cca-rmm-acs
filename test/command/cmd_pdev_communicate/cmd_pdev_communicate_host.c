/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_pdev_communicate_data.h"
#include "command_common_host.h"
#include "val_host_pcie.h"

static uint32_t num_bdf;

static struct argument_store {
    uint64_t pdev_ptr_valid;
    uint64_t data_ptr_valid;
    uint64_t req_addr_valid;
    uint64_t rsp_addr_valid;
    uint32_t bdf_valid;
    uint32_t doe_cap_base_valid;
    val_host_pdev_ts pdev_dev_valid;
    uint64_t pdev_ptr_error;
    val_host_pdev_ts pdev_dev_error;
} c_args;

struct arguments {
    uint64_t pdev_ptr;
    uint64_t data_ptr;
};

static uint64_t req_unaligned_prep_sequence(void)
{
    val_host_dev_comm_enter_ts *dev_comm_enter = NULL;
    val_host_dev_comm_data_ts *data_ptr = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    dev_comm_enter = &(((val_host_dev_comm_data_ts *)data_ptr)->enter);

    dev_comm_enter->req_addr = c_args.req_addr_valid + 1;
    dev_comm_enter->rsp_addr = c_args.rsp_addr_valid;
    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->rsp_len = 0;

    return (uint64_t)data_ptr;
}

static uint64_t rsp_unaligned_prep_sequence(void)
{
    val_host_dev_comm_enter_ts *dev_comm_enter = NULL;
    val_host_dev_comm_data_ts *data_ptr = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    dev_comm_enter = &(((val_host_dev_comm_data_ts *)data_ptr)->enter);

    dev_comm_enter->req_addr = c_args.req_addr_valid;
    dev_comm_enter->rsp_addr = c_args.rsp_addr_valid + 1;
    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->rsp_len = 0;

    return (uint64_t)data_ptr;
}

static uint64_t req_realm_pas_prep_sequence(void)
{
    val_host_dev_comm_enter_ts *dev_comm_enter = NULL;
    val_host_dev_comm_data_ts *data_ptr = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    dev_comm_enter = &(((val_host_dev_comm_data_ts *)data_ptr)->enter);

    dev_comm_enter->req_addr = g_delegated_prep_sequence();
    if (dev_comm_enter->req_addr == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_ERROR;
    dev_comm_enter->rsp_addr = c_args.rsp_addr_valid;
    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->rsp_len = 0;

    return (uint64_t)data_ptr;
}

static uint64_t req_secure_pas_prep_sequence(void)
{
    val_host_dev_comm_enter_ts *dev_comm_enter = NULL;
    val_host_dev_comm_data_ts *data_ptr = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    dev_comm_enter = &(((val_host_dev_comm_data_ts *)data_ptr)->enter);

    dev_comm_enter->req_addr = g_secure_prep_sequence();
    dev_comm_enter->rsp_addr = c_args.rsp_addr_valid;
    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->rsp_len = 0;

    return (uint64_t)data_ptr;
}

static uint64_t rsp_realm_pas_prep_sequence(void)
{
    val_host_dev_comm_enter_ts *dev_comm_enter = NULL;
    val_host_dev_comm_data_ts *data_ptr = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    dev_comm_enter = &(((val_host_dev_comm_data_ts *)data_ptr)->enter);

    dev_comm_enter->req_addr = c_args.req_addr_valid;
    dev_comm_enter->rsp_addr = g_delegated_prep_sequence();
    if (dev_comm_enter->rsp_addr == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_ERROR;
    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->rsp_len = 0;

    return (uint64_t)data_ptr;
}

static uint64_t rsp_secure_pas_prep_sequence(void)
{
    val_host_dev_comm_enter_ts *dev_comm_enter = NULL;
    val_host_dev_comm_data_ts *data_ptr = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    dev_comm_enter = &(((val_host_dev_comm_data_ts *)data_ptr)->enter);

    dev_comm_enter->req_addr = c_args.req_addr_valid;
    dev_comm_enter->rsp_addr = g_secure_prep_sequence();
    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->rsp_len = 0;

    return (uint64_t)data_ptr;
}

static uint64_t rsp_len_prep_sequence(void)
{
    val_host_dev_comm_enter_ts *dev_comm_enter = NULL;
    val_host_dev_comm_data_ts *data_ptr = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    dev_comm_enter = &(((val_host_dev_comm_data_ts *)data_ptr)->enter);

    dev_comm_enter->req_addr = c_args.req_addr_valid;
    dev_comm_enter->rsp_addr = c_args.rsp_addr_valid;
    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->rsp_len = PAGE_SIZE + 1;

    return (uint64_t)data_ptr;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    val_host_pdev_ts pdev_dev;
    val_host_pdev_flags_ts pdev_flags;

    val_host_dev_comm_data_ts *data_ptr = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    val_host_dev_comm_enter_ts *dev_comm_enter = NULL;
    uint64_t req_addr = 0;
    uint64_t rsp_addr = 0;

    val_memset(&pdev_dev, 0, sizeof(pdev_dev));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

    if (data_ptr == NULL)
        goto exit;

    dev_comm_enter = &data_ptr->enter;

    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR)
        goto exit;

    if (val_pcie_find_doe_capability(&num_bdf, &pdev_dev.bdf,
                                                     &pdev_dev.doe_cap_base))
        goto exit;

    val_host_set_pdev_flags(&pdev_flags);
    val_memcpy(&pdev_dev.pdev_flags, &pdev_flags, sizeof(pdev_flags));

    c_args.pdev_ptr_valid = g_pdev_new_prep_sequence(&pdev_dev);
    if (c_args.pdev_ptr_valid == VAL_TEST_PREP_SEQ_FAILED)
        goto exit;

    pdev_dev.cert_slot_id = 0;
    pdev_dev.cert_chain = val_host_mem_alloc(
                            PAGE_SIZE, VAL_HOST_PDEV_CERT_LEN_MAX);
    pdev_dev.cert_chain_len = 0;
    if (pdev_dev.cert_chain == NULL)
        goto exit;

    /* Allocate memory for req addr buffer */
    req_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!req_addr)
    {
        LOG(ERROR, "Failed to allocate memory for req_addr.\n");
        goto exit;
    }

    /* Allocate memory for rsp_addr buffer */
    rsp_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!rsp_addr)
    {
        LOG(ERROR, "Failed to allocate memory for rsp_addr.\n");
        goto exit;
    }

    c_args.data_ptr_valid = (uint64_t)data_ptr;
    c_args.req_addr_valid = req_addr;
    c_args.rsp_addr_valid = rsp_addr;
    c_args.bdf_valid = pdev_dev.bdf;
    c_args.doe_cap_base_valid = pdev_dev.doe_cap_base;
    pdev_dev.dev_comm_data = data_ptr;
    c_args.pdev_dev_valid = pdev_dev;

    dev_comm_enter->req_addr = req_addr;
    dev_comm_enter->rsp_addr = rsp_addr;
    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->rsp_len = 0;
    val_memset((uint64_t *)(dev_comm_enter->req_addr), 0, PAGE_SIZE);
    val_memset((uint64_t *)(dev_comm_enter->rsp_addr), 0, PAGE_SIZE);

    return VAL_SUCCESS;

exit:
    if (rsp_addr)
        val_host_mem_free((void *)rsp_addr);
    if (req_addr)
        val_host_mem_free((void *)req_addr);
    if (pdev_dev.cert_chain != NULL)
        val_host_mem_free(pdev_dev.cert_chain);
    if (data_ptr != NULL)
        val_host_mem_free(data_ptr);

    return VAL_TEST_PREP_SEQ_FAILED;
}

static uint64_t error_success_prep_sequence(void)
{
    val_host_pdev_flags_ts pdev_flags;

    val_memset(&c_args.pdev_dev_error, 0,
               sizeof(c_args.pdev_dev_error));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

    if (val_pcie_find_doe_capability(
            &num_bdf,
            &c_args.pdev_dev_error.bdf,
            &c_args.pdev_dev_error.doe_cap_base))
    {
        LOG(ERROR, "No additional DOE-capable PDEV found\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    val_host_set_pdev_flags(&pdev_flags);
    val_memcpy(&c_args.pdev_dev_error.pdev_flags, &pdev_flags,
               sizeof(pdev_flags));

    c_args.pdev_ptr_error =
        g_pdev_new_prep_sequence(&c_args.pdev_dev_error);

    return c_args.pdev_ptr_error;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case PDEV_UNALIGNED:
            args->pdev_ptr = g_unaligned_prep_sequence(c_args.pdev_ptr_valid);
            args->data_ptr = c_args.data_ptr_valid;
            break;


        case PDEV_GRAN_STATE_UNDELEGATED:
            args->pdev_ptr = g_undelegated_prep_sequence();
            args->data_ptr = c_args.data_ptr_valid;
            break;

        case DATA_UNALIGNED:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->data_ptr = g_unaligned_prep_sequence(c_args.data_ptr_valid);
            break;

        case DATA_PAS_REALM:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->data_ptr = g_delegated_prep_sequence();
            if (args->data_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case DATA_PAS_SECURE:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->data_ptr = g_secure_prep_sequence();
            break;

        case REQ_UNALIGNED:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->data_ptr = req_unaligned_prep_sequence();
            break;

        case REQ_PAS_REALM:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->data_ptr = req_realm_pas_prep_sequence();
            if (args->data_ptr == VAL_ERROR)
                return VAL_ERROR;
            break;

        case REQ_PAS_SECURE:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->data_ptr = req_secure_pas_prep_sequence();
            break;

        case RSP_UNALIGNED:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->data_ptr = rsp_unaligned_prep_sequence();
            break;

        case RSP_PAS_REALM:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->data_ptr = rsp_realm_pas_prep_sequence();
            if (args->data_ptr == VAL_ERROR)
                return VAL_ERROR;
            break;

        case RSP_PAS_SECURE:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->data_ptr = rsp_secure_pas_prep_sequence();
            break;

        case INVALID_RSP_LEN:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->data_ptr = rsp_len_prep_sequence();
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_pdev_communicate_host(void)
{
    uint64_t i;
    val_smc_param_ts cmd_ret;
    val_host_dev_comm_enter_ts *dev_comm_enter;
    struct arguments args;

    /* Skip if RMM do not support DA */
    if (!val_host_rmm_supports_da())
    {
        LOG(ALWAYS, "DA feature not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto exit;
    }

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_device;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "\n\tCheck %d : ", i + 1);
        LOG(TEST, test_data[i].msg);
        LOG(TEST, "; intent id : 0x%x \n", test_data[i].label);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto destroy_device;
        }

        cmd_ret = val_host_rmi_pdev_communicate(args.pdev_ptr, args.data_ptr);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_device;
        }
    }

    if (val_host_check_pdev_state(c_args.pdev_ptr_valid, RMI_PDEV_NEW))
    {
        LOG(ERROR, "Failed commands changed the valid PDEV state.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    LOG(TEST, "\n\tPositive Observability Check\n");
    if (val_host_dev_communicate(NULL, &c_args.pdev_dev_valid, NULL,
                                 RMI_PDEV_NEEDS_KEY))
    {
        LOG(ERROR, "PDEV communicate failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_device;
    }

    dev_comm_enter = &((val_host_dev_comm_data_ts *)c_args.data_ptr_valid)->enter;
    dev_comm_enter->req_addr = c_args.req_addr_valid;
    dev_comm_enter->rsp_addr = c_args.rsp_addr_valid;
    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->rsp_len = 0;

    cmd_ret = val_host_rmi_pdev_communicate(c_args.pdev_ptr_valid,
                                            c_args.data_ptr_valid);
    if (cmd_ret.x0 != RMI_ERROR_DEVICE)
    {
        LOG(ERROR, "Expected RMI_ERROR_DEVICE for DEV_COMM_IDLE, got 0x%x\n",
            cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_device;
    }

    LOG(TEST, "\n\tPositive Observability Check: error\n");

    if (error_success_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_device;
    }

    dev_comm_enter = &c_args.pdev_dev_error.dev_comm_data->enter;
    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->rsp_len = 0;

    cmd_ret = val_host_rmi_pdev_communicate(
                                c_args.pdev_ptr_error,
                                (uint64_t)c_args.pdev_dev_error.dev_comm_data);
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "Failed to start PDEV communication, ret=0x%x\n",
            cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_device;
    }

    /* Report a device error after the transaction becomes active. */
    dev_comm_enter->status = RMI_DEV_COMM_ERROR;
    dev_comm_enter->rsp_len = 0;

    cmd_ret = val_host_rmi_pdev_communicate(
                                c_args.pdev_ptr_error,
                                (uint64_t)c_args.pdev_dev_error.dev_comm_data);
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_PDEV_COMMUNICATE returned 0x%x, expected RMI_SUCCESS\n",
            cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_device;
    }

    cmd_ret = val_host_rmi_pdev_get_state(c_args.pdev_ptr_error);
    if ((cmd_ret.x0 != RMI_SUCCESS) || (cmd_ret.x1 != RMI_PDEV_ERROR))
    {
        LOG(ERROR, "Expected PDEV_ERROR, got state 0x%x\n", cmd_ret.x1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto destroy_device;
    }

    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->rsp_len = 0;

    cmd_ret = val_host_rmi_pdev_communicate(
                                c_args.pdev_ptr_error,
                                (uint64_t)c_args.pdev_dev_error.dev_comm_data);
    if (cmd_ret.x0 != RMI_ERROR_DEVICE)
    {
        LOG(ERROR, "Expected RMI_ERROR_DEVICE for DEV_COMM_ERROR, got 0x%x\n",
            cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_device;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    if (c_args.pdev_ptr_error &&
        val_host_pdev_teardown(&c_args.pdev_dev_error,
                               c_args.pdev_ptr_error))
    {
        LOG(ERROR, "Error-path PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
    }

    if (c_args.pdev_ptr_valid &&
        val_host_pdev_teardown(&c_args.pdev_dev_valid, c_args.pdev_ptr_valid))
    {
        LOG(ERROR, "Valid PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto exit;
    }

exit:
    return;
}
