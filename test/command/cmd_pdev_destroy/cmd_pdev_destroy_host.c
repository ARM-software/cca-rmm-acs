/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_pdev_destroy_data.h"
#include "command_common_host.h"
#include "val_host_pcie.h"
#include "val_host_doe.h"

static uint32_t num_bdf;

static struct argument_store {
    uint64_t pdev_ptr_valid;
} c_args;

static struct invalid_argument_store {
    uint64_t pdev_new_ptr;
    val_host_pdev_ts pdev_dev_new;
} c_args_invalid;


struct arguments {
    uint64_t pdev_ptr;
};

static uint64_t pdev_new_prep_sequence(void)
{
    val_host_pdev_ts pdev_dev;
    val_host_pdev_flags_ts pdev_flags;
    uint32_t rp_bdf, status;

    val_memset(&pdev_dev, 0, sizeof(pdev_dev));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

    if (val_pcie_find_doe_capability(&num_bdf, &pdev_dev.bdf,
                                                     &pdev_dev.doe_cap_base))
        return VAL_TEST_PREP_SEQ_FAILED;

    status = val_pcie_get_rootport((uint32_t)pdev_dev.bdf, &rp_bdf);
    if (status != 0)
        return VAL_TEST_PREP_SEQ_FAILED;

    pdev_dev.root_id = (uint16_t)rp_bdf;

    val_host_set_pdev_flags(&pdev_flags);
    val_memcpy(&pdev_dev.pdev_flags, &pdev_flags, sizeof(pdev_flags));

    pdev_dev.pdev = g_pdev_new_prep_sequence(&pdev_dev);
    if (c_args.pdev_ptr_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args_invalid.pdev_dev_new = pdev_dev;
    c_args_invalid.pdev_new_ptr = pdev_dev.pdev;

    return pdev_dev.pdev;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    val_smc_param_ts args;
    uint64_t pdev, req_addr, resp_addr;
    val_host_pdev_ts pdev_dev;
    val_host_pdev_flags_ts pdev_flags;
    uint32_t status, rp_bdf;

    val_memset(&pdev_dev, 0, sizeof(pdev_dev));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR)
        return VAL_TEST_PREP_SEQ_FAILED;

    if (val_pcie_find_doe_capability(&num_bdf, (uint32_t *)&pdev_dev.bdf,
                                                     &pdev_dev.doe_cap_base))
        return VAL_TEST_PREP_SEQ_FAILED;

    status = val_pcie_get_rootport((uint32_t)pdev_dev.bdf, &rp_bdf);
    if (status != 0)
        return VAL_TEST_PREP_SEQ_FAILED;

    pdev_dev.root_id = (uint16_t)rp_bdf;

    val_host_set_pdev_flags(&pdev_flags);
    val_memcpy(&pdev_dev.pdev_flags, &pdev_flags, sizeof(pdev_flags));

    pdev = g_pdev_new_prep_sequence(&pdev_dev);
    if (pdev == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    args = val_host_rmi_pdev_stop(pdev);
    if (args.x0)
    {
        LOG(ERROR, "PDEV stop failed.\n");
        goto exit;
    }

    /* PDEV state should be PDEV_STOPPINGG */
    if (val_host_check_pdev_state(pdev, RMI_PDEV_STOPPING))
    {
        LOG(ERROR, "PDEV state check failed.\n");
        goto exit;
    }

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

    /* Allocate buffer to cache VCA */
    pdev_dev.vca = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_PDEV_VCA_LEN_MAX);
    pdev_dev.vca_len = 0;
    if (pdev_dev.vca == NULL) {
        goto exit;
    }

    pdev_dev.pdev = pdev;
    pdev_dev.dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    pdev_dev.dev_comm_data->enter.req_addr = req_addr;
    pdev_dev.dev_comm_data->enter.resp_addr = resp_addr;

    if (val_host_dev_communicate(NULL, &pdev_dev, NULL, RMI_PDEV_STOPPED))
    {
        LOG(ERROR, "PDEV communicate failed.\n");
        goto exit;
    }

    if (val_host_check_pdev_state(pdev, RMI_PDEV_STOPPED))
    {
        LOG(ERROR, "PDEV state check failed.\n");
        goto exit;
    }

    c_args.pdev_ptr_valid =  pdev;

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
            break;

        case PDEV_OUTSIDE_OF_PERMITTED_PA:
            args->pdev_ptr = g_outside_of_permitted_pa_prep_sequence();
            break;

        case PDEV_DEV_MEM_MMIO:
            args->pdev_ptr = g_dev_mem_prep_sequence();
            break;

        case PDEV_GRAN_STATE_UNDELEGATED:
            args->pdev_ptr = g_undelegated_prep_sequence();
            break;

        case PDEV_STATE_PDEV_NEW:
            args->pdev_ptr = pdev_new_prep_sequence();
            if (args->pdev_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_TEST_PREP_SEQ_FAILED;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_pdev_destroy_host(void)
{
    uint64_t i;
    val_smc_param_ts cmd_ret;
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

        cmd_ret = val_host_rmi_pdev_destroy(args.pdev_ptr);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_device;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n");
    cmd_ret = val_host_rmi_pdev_destroy(c_args.pdev_ptr_valid);
    if (cmd_ret.x0 != RMI_SUCCESS
    )
    {
        LOG(ERROR, "Command failed. %x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    if (c_args_invalid.pdev_new_ptr &&
        val_host_pdev_teardown(&c_args_invalid.pdev_dev_new,
                               c_args_invalid.pdev_new_ptr))
    {
        LOG(ERROR, "PDEV teardown failed.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

exit:
    return;
}
