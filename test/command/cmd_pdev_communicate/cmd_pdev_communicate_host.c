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
    uint64_t resp_addr_valid;
    uint32_t bdf_valid;
    uint32_t doe_cap_base_valid;
    val_host_pdev_ts pdev_dev_valid;
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
    dev_comm_enter->resp_addr = c_args.resp_addr_valid;
    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->resp_len = 0;

    return (uint64_t)data_ptr;
}

static uint64_t resp_unaligned_prep_sequence(void)
{
    val_host_dev_comm_enter_ts *dev_comm_enter = NULL;
    val_host_dev_comm_data_ts *data_ptr = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    dev_comm_enter = &(((val_host_dev_comm_data_ts *)data_ptr)->enter);

    dev_comm_enter->req_addr = c_args.req_addr_valid;
    dev_comm_enter->resp_addr = c_args.resp_addr_valid + 1;
    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->resp_len = 0;

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
    dev_comm_enter->resp_addr = c_args.resp_addr_valid;
    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->resp_len = 0;

    return (uint64_t)data_ptr;
}

static uint64_t req_secure_pas_prep_sequence(void)
{
    val_host_dev_comm_enter_ts *dev_comm_enter = NULL;
    val_host_dev_comm_data_ts *data_ptr = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    dev_comm_enter = &(((val_host_dev_comm_data_ts *)data_ptr)->enter);

    dev_comm_enter->req_addr = g_secure_prep_sequence();
    dev_comm_enter->resp_addr = c_args.resp_addr_valid;
    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->resp_len = 0;

    return (uint64_t)data_ptr;
}

static uint64_t resp_realm_pas_prep_sequence(void)
{
    val_host_dev_comm_enter_ts *dev_comm_enter = NULL;
    val_host_dev_comm_data_ts *data_ptr = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    dev_comm_enter = &(((val_host_dev_comm_data_ts *)data_ptr)->enter);

    dev_comm_enter->req_addr = c_args.req_addr_valid;
    dev_comm_enter->resp_addr = g_delegated_prep_sequence();
    if (dev_comm_enter->resp_addr == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_ERROR;
    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->resp_len = 0;

    return (uint64_t)data_ptr;
}

static uint64_t resp_secure_pas_prep_sequence(void)
{
    val_host_dev_comm_enter_ts *dev_comm_enter = NULL;
    val_host_dev_comm_data_ts *data_ptr = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    dev_comm_enter = &(((val_host_dev_comm_data_ts *)data_ptr)->enter);

    dev_comm_enter->req_addr = c_args.req_addr_valid;
    dev_comm_enter->resp_addr = g_secure_prep_sequence();
    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->resp_len = 0;

    return (uint64_t)data_ptr;
}

static uint64_t resp_len_prep_sequence(void)
{
    val_host_dev_comm_enter_ts *dev_comm_enter = NULL;
    val_host_dev_comm_data_ts *data_ptr = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    dev_comm_enter = &(((val_host_dev_comm_data_ts *)data_ptr)->enter);

    dev_comm_enter->req_addr = c_args.req_addr_valid;
    dev_comm_enter->resp_addr = c_args.resp_addr_valid;
    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->resp_len = PAGE_SIZE + 1;

    return (uint64_t)data_ptr;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    val_host_pdev_ts pdev_dev;
    val_host_pdev_flags_ts pdev_flags;

    val_host_dev_comm_data_ts *data_ptr = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    val_host_dev_comm_enter_ts *dev_comm_enter = NULL;
    uint64_t req_addr, resp_addr;

    dev_comm_enter = &(((val_host_dev_comm_data_ts *)data_ptr)->enter);

    val_memset(&pdev_dev, 0, sizeof(pdev_dev));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

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
        return VAL_TEST_PREP_SEQ_FAILED;

    /* Allocate memory for req addr buffer */
    req_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!req_addr)
    {
        LOG(ERROR, "Failed to allocate memory for req_addr.\n");
        goto exit;
    }

    /* Allocate memory for resp_addr buffer */
    resp_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!resp_addr)
    {
        LOG(ERROR, "Failed to allocate memory for resp_addr.\n");
        goto exit;
    }

    c_args.data_ptr_valid = (uint64_t)data_ptr;
    c_args.req_addr_valid = req_addr;
    c_args.resp_addr_valid = resp_addr;
    c_args.bdf_valid = pdev_dev.bdf;
    c_args.doe_cap_base_valid = pdev_dev.doe_cap_base;
    c_args.pdev_dev_valid = pdev_dev;

    dev_comm_enter->req_addr = req_addr;
    dev_comm_enter->resp_addr = resp_addr;
    dev_comm_enter->status = RMI_DEV_COMM_NONE;
    dev_comm_enter->resp_len = 0;
    val_memset((uint64_t *)(dev_comm_enter->req_addr), 0, PAGE_SIZE);
    val_memset((uint64_t *)(dev_comm_enter->resp_addr), 0, PAGE_SIZE);

    return VAL_SUCCESS;

exit:
    return VAL_TEST_PREP_SEQ_FAILED;
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

        case PDEV_OUTSIDE_OF_PERMITTED_PA:
            args->pdev_ptr = g_outside_of_permitted_pa_prep_sequence();
            args->data_ptr = c_args.data_ptr_valid;
            break;

        case PDEV_DEV_MEM_MMIO:
            args->pdev_ptr = g_dev_mem_prep_sequence();
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

        case RESP_UNALIGNED:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->data_ptr = resp_unaligned_prep_sequence();
            break;

        case RESP_PAS_REALM:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->data_ptr = resp_realm_pas_prep_sequence();
            if (args->data_ptr == VAL_ERROR)
                return VAL_ERROR;
            break;

        case RESP_PAS_SECURE:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->data_ptr = resp_secure_pas_prep_sequence();
            break;

        case INVALID_RESP_LEN:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->data_ptr = resp_len_prep_sequence();
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
    struct arguments args;
    val_host_pdev_ts pdev_dev;
    val_host_pdev_flags_ts pdev_flags;
    uint32_t rp_bdf, status;

    val_memset(&pdev_dev, 0, sizeof(pdev_dev));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

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

    LOG(TEST, "\n\tPositive Observability Check\n");

    cmd_ret = val_host_rmi_pdev_stop(c_args.pdev_ptr_valid);
    if (cmd_ret.x0) {
        LOG(ERROR, "PDEV stop failed, ret=0x%lx\n", (unsigned long)cmd_ret.x0);
    }

    if (val_host_dev_communicate(NULL, &c_args.pdev_dev_valid, NULL, RMI_PDEV_STOPPED))
    {
        LOG(ERROR, "Command failed.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    /* Success condition check */
    if (val_pcie_find_doe_capability(&num_bdf, (uint32_t *)&pdev_dev.bdf,
                                                     &pdev_dev.doe_cap_base))
        goto destroy_device;

    status = val_pcie_get_rootport((uint32_t)pdev_dev.bdf, &rp_bdf);
    if (status != 0)
        goto destroy_device;

    pdev_dev.root_id = (uint16_t)rp_bdf;

    val_host_set_pdev_flags(&pdev_flags);
    val_memcpy(&pdev_dev.pdev_flags, &pdev_flags, sizeof(pdev_flags));

    pdev_dev.pdev = g_pdev_ready_prep_sequence(&pdev_dev);
    if (pdev_dev.pdev == VAL_TEST_PREP_SEQ_FAILED)
        goto destroy_device;

    /* PDEV state should be PDEV_READY */
    if (val_host_check_pdev_state(pdev_dev.pdev, RMI_PDEV_READY))
    {
        LOG(ERROR, "PDEV state check failed.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_device;
    }

    cmd_ret = val_host_rmi_pdev_ide_reset(pdev_dev.pdev);
    if (cmd_ret.x0)
    {
        LOG(ERROR, "PDEV IDE key refresh failed.\n");
        goto destroy_device;
    }

    /* PDEV state should be PDEV_IDE_RESETTING */
    if (val_host_check_pdev_state(pdev_dev.pdev, RMI_PDEV_IDE_RESETTING))
    {
        LOG(ERROR, "PDEV state check failed.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_device;
    }

    if (val_host_dev_communicate(NULL, &pdev_dev, NULL, RMI_PDEV_READY))
    {
        LOG(ERROR, "PDEV communicate failed.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_device;
    }

    /* PDEV state should be PDEV_READY */
    if (val_host_check_pdev_state(pdev_dev.pdev, RMI_PDEV_READY))
    {
        LOG(ERROR, "PDEV state check failed.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_device;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    if (pdev_dev.pdev &&
        val_host_pdev_teardown(&pdev_dev, pdev_dev.pdev))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

    if (c_args.pdev_ptr_valid &&
        val_host_pdev_teardown(&c_args.pdev_dev_valid, c_args.pdev_ptr_valid))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto exit;
    }

exit:
    return;
}
