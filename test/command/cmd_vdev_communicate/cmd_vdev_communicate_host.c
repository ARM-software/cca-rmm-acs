/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_vdev_communicate_data.h"
#include "command_common_host.h"
#include "val_host_pcie.h"

static uint32_t num_bdf;

static struct argument_store {
    uint64_t pdev_ptr_valid;
    uint64_t vdev_ptr_valid;
    uint64_t data_ptr_valid;
    uint64_t req_addr_valid;
    uint64_t resp_addr_valid;
    uint64_t rd_valid;
    val_host_vdev_ts vdev_dev_valid;
    val_host_pdev_ts pdev_dev_valid;
    val_host_realm_ts realm_valid;
} c_args;

static struct invalid_argument_store {
    uint64_t pdev_ptr;
    val_host_pdev_ts pdev_dev_invalid;
} c_args_invalid;

struct arguments {
    uint64_t rd;
    uint64_t pdev_ptr;
    uint64_t vdev_ptr;
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

static uint64_t invalid_pdev_prep_sequence(void)
{
    val_host_pdev_ts pdev_dev;
    val_host_pdev_flags_ts pdev_flags;
    uint32_t rp_bdf, status;

    val_memset(&pdev_dev, 0, sizeof(pdev_dev));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

    if (val_pcie_find_doe_capability(&num_bdf, (uint32_t *)&pdev_dev.bdf,
                                                     &pdev_dev.doe_cap_base))
        return VAL_TEST_PREP_SEQ_FAILED;

    status = val_pcie_get_rootport((uint32_t)pdev_dev.bdf, &rp_bdf);
    if (status != 0)
        return VAL_TEST_PREP_SEQ_FAILED;

    pdev_dev.root_id = (uint16_t)rp_bdf;

    val_host_set_pdev_flags(&pdev_flags);
    val_memcpy(&pdev_dev.pdev_flags, &pdev_flags, sizeof(pdev_flags));

    pdev_dev.pdev = g_pdev_ready_prep_sequence(&pdev_dev);
    if (pdev_dev.pdev == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args_invalid.pdev_ptr = pdev_dev.pdev;
    c_args_invalid.pdev_dev_invalid = pdev_dev;

    return pdev_dev.pdev;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    val_host_pdev_ts pdev_dev;
    val_host_vdev_ts vdev_dev;
    val_host_realm_ts realm;
    val_host_pdev_flags_ts pdev_flags;
    uint32_t rp_bdf, status;
    val_smc_param_ts args;

    val_memset(&pdev_dev, 0, sizeof(pdev_dev));
    val_memset(&vdev_dev, 0, sizeof(vdev_dev));
    val_memset(&realm, 0, sizeof(realm));
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

    c_args.vdev_ptr_valid = g_vdev_new_prep_sequence(&pdev_dev, &vdev_dev, &realm);
    if (vdev_dev.vdev == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    args = val_host_rmi_vdev_get_state(c_args.vdev_ptr_valid);
    if (args.x0)
    {
        LOG(ERROR, "VDEV get state failed ret %lx\n", args.x0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Check VDEV state is RMI_VDEV_NEW */
    if (args.x1 != RMI_VDEV_NEW)
    {
        LOG(ERROR, "VDEV state should be RMI_VDEV_NEW, ret %lx\n", args.x1);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    c_args.pdev_ptr_valid = pdev_dev.pdev;
    c_args.rd_valid = vdev_dev.realm_rd;
    c_args.vdev_dev_valid = vdev_dev;
    c_args.pdev_dev_valid = pdev_dev;
    c_args.realm_valid = realm;
    c_args.req_addr_valid = vdev_dev.dev_comm_data->enter.req_addr;
    c_args.resp_addr_valid = vdev_dev.dev_comm_data->enter.resp_addr;
    c_args.data_ptr_valid = (uint64_t)vdev_dev.dev_comm_data;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->data_ptr = c_args.data_ptr_valid;
            break;

        case RD_OUTSIDE_OF_PERMITTED_PA:
            args->rd = g_outside_of_permitted_pa_prep_sequence();
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->data_ptr = c_args.data_ptr_valid;
            break;

        case RD_DEV_MEM_MMIO:
            args->rd = g_dev_mem_prep_sequence();
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->data_ptr = c_args.data_ptr_valid;
            break;

        case RD_GRAN_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->data_ptr = c_args.data_ptr_valid;
            break;

        case PDEV_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = g_unaligned_prep_sequence(c_args.pdev_ptr_valid);
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->data_ptr = c_args.data_ptr_valid;
            break;

        case PDEV_OUTSIDE_OF_PERMITTED_PA:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = g_outside_of_permitted_pa_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->data_ptr = c_args.data_ptr_valid;
            break;

        case PDEV_DEV_MEM_MMIO:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = g_dev_mem_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->data_ptr = c_args.data_ptr_valid;
            break;

        case PDEV_GRAN_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = g_undelegated_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->data_ptr = c_args.data_ptr_valid;
            break;

        case VDEV_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = g_unaligned_prep_sequence(c_args.vdev_ptr_valid);
            args->data_ptr = c_args.data_ptr_valid;
            break;

        case VDEV_OUTSIDE_OF_PERMITTED_PA:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = g_outside_of_permitted_pa_prep_sequence();
            args->data_ptr = c_args.data_ptr_valid;
            break;

        case VDEV_DEV_MEM_MMIO:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = g_dev_mem_prep_sequence();
            args->data_ptr = c_args.data_ptr_valid;
            break;

        case VDEV_GRAN_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = g_undelegated_prep_sequence();
            args->data_ptr = c_args.data_ptr_valid;
            break;

        case DATA_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->data_ptr = g_unaligned_prep_sequence(c_args.data_ptr_valid);
            break;

        case DATA_PAS_REALM:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->data_ptr = g_delegated_prep_sequence();
            if (args->data_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case DATA_PAS_SECURE:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->data_ptr = g_secure_prep_sequence();
            break;

        case REQ_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->data_ptr = req_unaligned_prep_sequence();
            break;

        case REQ_PAS_REALM:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->data_ptr = req_realm_pas_prep_sequence();
            if (args->data_ptr == VAL_ERROR)
                return VAL_ERROR;
            break;

        case REQ_PAS_SECURE:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->data_ptr = req_secure_pas_prep_sequence();
            break;

        case RESP_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->data_ptr = resp_unaligned_prep_sequence();
            break;

        case RESP_PAS_REALM:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->data_ptr = resp_realm_pas_prep_sequence();
            if (args->data_ptr == VAL_ERROR)
                return VAL_ERROR;
            break;

        case RESP_PAS_SECURE:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->data_ptr = resp_secure_pas_prep_sequence();
            break;

        case INVALID_RESP_LEN:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->data_ptr = resp_len_prep_sequence();
            break;

        case INVALID_PDEV:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = invalid_pdev_prep_sequence();
            if (args->pdev_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_TEST_PREP_SEQ_FAILED;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->data_ptr = c_args.data_ptr_valid;
            break;

        case VDEV_GRAN_STATE_UNDELEGATED_INVALID_PDEV:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args_invalid.pdev_ptr;
            args->vdev_ptr = g_undelegated_prep_sequence();
            args->data_ptr = c_args.data_ptr_valid;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_vdev_communicate_host(void)
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

        cmd_ret = val_host_rmi_vdev_communicate(args.rd, args.pdev_ptr, args.vdev_ptr,
                                                args.data_ptr);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_device;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n");
    cmd_ret = val_host_rmi_vdev_communicate(c_args.rd_valid, c_args.pdev_ptr_valid,
                                     c_args.vdev_ptr_valid, c_args.data_ptr_valid);
    if (cmd_ret.x0 != 0)
    {
        LOG(ERROR, "Command failed. %x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    cmd_ret = val_host_rmi_vdev_get_state(c_args.vdev_ptr_valid);
    if (cmd_ret.x0)
    {
        LOG(ERROR, "VDEV get state failed ret %lx\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_device;
    }

    /* Check VDEV state is RMI_VDEV_UNLOCK */
    if (cmd_ret.x1 != RMI_VDEV_UNLOCKED)
    {
        LOG(ERROR, "VDEV state should be RMI_VDEV_UNLOCK, ret %lx\n", cmd_ret.x1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_device;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    if (c_args.vdev_ptr_valid &&
        val_host_vdev_teardown(&c_args.realm_valid, &c_args.pdev_dev_valid,
                               &c_args.vdev_dev_valid))
    {
        LOG(ERROR, "VDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    if (c_args.pdev_ptr_valid &&
        val_host_pdev_teardown(&c_args.pdev_dev_valid, c_args.pdev_ptr_valid))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

    if (c_args_invalid.pdev_ptr &&
        val_host_pdev_teardown(&c_args_invalid.pdev_dev_invalid, c_args_invalid.pdev_ptr))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto exit;
    }

exit:
    return;
}
