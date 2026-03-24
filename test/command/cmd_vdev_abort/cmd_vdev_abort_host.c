/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_vdev_abort_data.h"
#include "command_common_host.h"
#include "val_host_pcie.h"

#define VALID_REALM   1

static uint32_t num_bdf;

static struct invalid_argument_store {
    uint64_t pdev;
    val_host_pdev_ts pdev_dev_invalid;
} c_args_invalid;

static struct argument_store {
    uint64_t rd_valid;
    uint64_t pdev_ptr_valid;
    uint64_t vdev_ptr_valid;
    val_host_vdev_ts vdev_dev_valid;
    val_host_pdev_ts pdev_dev_valid;
    val_host_realm_ts realm_valid;
} c_args;

struct arguments {
    uint64_t rd;
    uint64_t pdev_ptr;
    uint64_t vdev_ptr;
};

static uint64_t rd_new_prep_sequence(void)
{
    val_host_realm_ts realm;

    val_memset(&realm, 0, sizeof(realm));

    realm.s2sz = 40;
    realm.hash_algo = RMI_HASH_SHA_256;
    realm.s2_starting_level = 0;
    realm.num_s2_sl_rtts = 1;
    realm.vmid = VALID_REALM;

    if (val_host_realm_create_common(&realm))
    {
        LOG(ERROR, "\tRealm create failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm.rd;
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

    c_args_invalid.pdev = pdev_dev.pdev;
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

    c_args.pdev_ptr_valid = pdev_dev.pdev;
    c_args.rd_valid = vdev_dev.realm_rd;
    c_args.vdev_dev_valid = vdev_dev;
    c_args.pdev_dev_valid = pdev_dev;
    c_args.realm_valid = realm;

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
            break;

        case RD_OUTSIDE_OF_PERMITTED_PA:
            args->rd = g_outside_of_permitted_pa_prep_sequence();
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            break;

        case RD_DEV_MEM_MMIO:
            args->rd = g_dev_mem_prep_sequence();
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            break;

        case RD_GRAN_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            break;

        case PDEV_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = g_unaligned_prep_sequence(c_args.pdev_ptr_valid);
            args->vdev_ptr = c_args.vdev_ptr_valid;
            break;

        case PDEV_OUTSIDE_OF_PERMITTED_PA:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = g_outside_of_permitted_pa_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            break;

        case PDEV_DEV_MEM_MMIO:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = g_dev_mem_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            break;

        case PDEV_GRAN_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = g_undelegated_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            break;

        case VDEV_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = g_unaligned_prep_sequence(c_args.vdev_ptr_valid);
            break;

        case VDEV_OUTSIDE_OF_PERMITTED_PA:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = g_outside_of_permitted_pa_prep_sequence();
            break;

        case VDEV_DEV_MEM_MMIO:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = g_dev_mem_prep_sequence();
            break;

        case VDEV_GRAN_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = g_undelegated_prep_sequence();
            break;

        case INVALID_RD:
            args->rd = rd_new_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_TEST_PREP_SEQ_FAILED;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            break;

        case INVALID_PDEV:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = invalid_pdev_prep_sequence();
            if (args->pdev_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_TEST_PREP_SEQ_FAILED;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_vdev_abort_host(void)
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

        /*TODO: Update once TF-RMM correct vdev_abort command */
        // cmd_ret = val_host_rmi_vdev_abort(args.rd, args.pdev_ptr, args.vdev_ptr);
        cmd_ret = val_host_rmi_vdev_abort(args.vdev_ptr);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_device;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n");
    /*TODO: Update once TF-RMM correct vdev_abort command */
    // cmd_ret = val_host_rmi_vdev_abort(c_args.rd_valid, c_args.pdev_ptr_valid,
                                                     //  c_args.vdev_ptr_valid);
    cmd_ret = val_host_rmi_vdev_abort(c_args.vdev_ptr_valid);
    if (cmd_ret.x0 != 0)
    {
        LOG(ERROR, "Command failed. %x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    cmd_ret = val_host_rmi_vdev_get_state(c_args.vdev_ptr_valid);
    if (cmd_ret.x0)
    {
        LOG(ERROR, "VDEV_GET_STATE failed ret %lx.\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_device;
    }

    /* Check VDEV state is RMI_VDEV_ERROR */
    if (cmd_ret.x1 != RMI_VDEV_ERROR)
    {
        LOG(ERROR, "VDEV state should be RMI_VDEV_ERROR, ret %lx\n", cmd_ret.x1);
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
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    if (c_args.pdev_ptr_valid &&
        val_host_pdev_teardown(&c_args.pdev_dev_valid, c_args.pdev_ptr_valid))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    if (c_args_invalid.pdev &&
        val_host_pdev_teardown(&c_args_invalid.pdev_dev_invalid, c_args_invalid.pdev))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

exit:
    return;
}
