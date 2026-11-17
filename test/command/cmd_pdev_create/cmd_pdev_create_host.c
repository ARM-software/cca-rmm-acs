/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_rmi_sro.h"
#include "val_host_sro.h"
#include "rmi_pdev_create_data.h"
#include "command_common_host.h"
#include "val_host_da.h"
#include "val_host_pcie.h"

static uint32_t num_bdf;

static struct argument_store {
    uint64_t pdev_ptr_valid;
    uint64_t params_ptr_valid;
    uint64_t featreg2;
    val_host_pdev_ts pdev_dev_valid;
    val_host_tracking_test_region_ts track_region;
} c_args;

struct arguments {
    uint64_t pdev_ptr;
    uint64_t params_ptr;
    uint64_t params_ptr_allocated;
};

static uint64_t pdev_track_prep_sequence(void)
{
    return val_host_tracking_coarse_prep_sequence(&c_args.track_region);
}

static uint32_t pdev_track_cleanup(void)
{
    return val_host_tracking_coarse_cleanup(&c_args.track_region);
}

static uint64_t clone_valid_params(struct arguments *args)
{
    void *params;

    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (params == NULL)
    {
        LOG(ERROR, "Failed to allocate PDEV parameters.\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    val_memcpy(params, (void *)c_args.params_ptr_valid, PAGE_SIZE);
    args->params_ptr = (uint64_t)params;
    args->params_ptr_allocated = (uint64_t)params;

    return VAL_SUCCESS;
}

static uint64_t params_prep_sequence(void)
{
    val_host_rmi_pdev_params_ts *pdev_params;
    val_host_pdev_flags_ts pdev_flags;
    val_host_pdev_ts pdev_dev;

    val_memset(&pdev_dev, 0, sizeof(pdev_dev));

    /* Allocate memory for params */
    pdev_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev_params == NULL)
    {
        LOG(ERROR, "Failed to allocate memory for pdev_params.\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    val_memset(pdev_params, 0, PAGE_SIZE_4K);
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

    val_host_set_pdev_flags(&pdev_flags);

    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR)
        return VAL_TEST_PREP_SEQ_FAILED;

    if (val_pcie_find_doe_capability(&num_bdf, &pdev_dev.bdf,
                                                     &pdev_dev.doe_cap_base))
        return VAL_TEST_PREP_SEQ_FAILED;

    /* Populate params */
    val_memcpy(&pdev_params->flags, &pdev_flags, sizeof(pdev_flags));
    pdev_params->hb_base = val_pcie_get_ecam_base(pdev_dev.bdf);
    pdev_params->pdev_id = PCIE_CREATE_BDF_PACKED(pdev_dev.bdf);
    pdev_params->routing_id = PCIE_EXTRACT_BDF_SEG(pdev_dev.bdf);
    pdev_params->id_index = 0;
    pdev_params->rid_base = PCIE_CREATE_BDF_PACKED(pdev_dev.bdf);
    pdev_params->rid_top = pdev_params->rid_base + 1;
    pdev_params->hash_algo = RMI_HASH_SHA_256;

    pdev_dev.pdev = c_args.pdev_ptr_valid;
    val_memcpy(&pdev_dev.pdev_flags, &pdev_flags, sizeof(pdev_flags));
    c_args.pdev_dev_valid = pdev_dev;

    return (uint64_t)pdev_params;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    uint64_t pdev, ret;

    /* Allocate and delegate PDEV */
    pdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!pdev)
    {
        LOG(ERROR, "Failed to allocate memory for pdev.\n");
        goto exit;
    } else {
        ret = val_host_rmi_granule_range_delegate(pdev, pdev + PAGE_SIZE);
        if (ret)
        {
            LOG(ERROR, "PDEV delegation failed, pdev=0x%x, ret=0x%x.\n", pdev, ret);
            goto exit;
        }
    }

    c_args.pdev_ptr_valid = pdev;

    c_args.params_ptr_valid = params_prep_sequence();
    if (c_args.params_ptr_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    if (val_host_rmi_features(RMI_FEATURE_REGISTER_2_INDEX,
                              &c_args.featreg2))
    {
        LOG(ERROR, "Failed to read RMI feature register 2.\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return VAL_SUCCESS;

exit:
    return VAL_TEST_PREP_SEQ_FAILED;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;
    val_host_rmi_pdev_params_ts *pdev_params;
    val_host_pdev_flags_ts *pdev_flags;
    uint64_t max_vdevs_order;

    args->params_ptr_allocated = 0U;

    switch (label)
    {
        case PDEV_UNALIGNED:
            args->pdev_ptr = g_unaligned_prep_sequence(c_args.pdev_ptr_valid);
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case PDEV_TRACK_COARSE:
            args->params_ptr = c_args.params_ptr_valid;
            args->pdev_ptr = pdev_track_prep_sequence();
            if (args->pdev_ptr == VAL_SKIP_CHECK)
                return VAL_SKIP_CHECK;
            if (args->pdev_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case PDEV_GRAN_STATE_UNDELEGATED:
            args->pdev_ptr = g_undelegated_prep_sequence();
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case PARAMS_UNALIGNED:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = g_unaligned_prep_sequence(c_args.params_ptr_valid);
            break;

        case PARAMS_PAS_REALM:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = g_delegated_prep_sequence();
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case PARAMS_PAS_SECURE:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = g_secure_prep_sequence();
            break;

        case PARAMS_INVALID:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            if (clone_valid_params(args))
                return VAL_ERROR;
            pdev_params = (val_host_rmi_pdev_params_ts *)args->params_ptr;
            pdev_params->hash_algo = 0xFFU;
            break;

        case HB_BASE_INVALID:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            if (clone_valid_params(args))
                return VAL_ERROR;
            pdev_params = (val_host_rmi_pdev_params_ts *)args->params_ptr;
            pdev_params->hb_base = ~0UL;
            break;

        case PDEV_ID_INVALID:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            if (clone_valid_params(args))
                return VAL_ERROR;
            pdev_params = (val_host_rmi_pdev_params_ts *)args->params_ptr;
            pdev_params->pdev_id = ~0UL;
            break;

        case FLAGS_UNSUPPORTED:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            if (clone_valid_params(args))
                return VAL_ERROR;
            pdev_params = (val_host_rmi_pdev_params_ts *)args->params_ptr;
            pdev_flags = (val_host_pdev_flags_ts *)&pdev_params->flags;
            pdev_flags->spdm = RMI_SPDM_FALSE;
            break;

        case MAX_NUM_VDEVS:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            if (clone_valid_params(args))
                return VAL_ERROR;
            pdev_params = (val_host_rmi_pdev_params_ts *)args->params_ptr;
            max_vdevs_order = VAL_EXTRACT_BITS(
                    c_args.featreg2,
                    RMI_FEATURE_REGISTER_2_MAX_VDEVS_ORDER_START,
                    RMI_FEATURE_REGISTER_2_MAX_VDEVS_ORDER_END);
            pdev_params->rid_top = pdev_params->rid_base +
                                   (1U << max_vdevs_order);
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_pdev_create_host(void)
{
    uint64_t i;
    uint64_t ret;
    val_smc_param_ts cmd_ret;
    struct arguments args = {0};
    bool pdev_created = false;

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

        ret = intent_to_seq(&test_data[i], &args);
        if (ret == VAL_SKIP_CHECK)
        {
            LOG(TEST, "Skipping Check %d\n", (int)(i + 1));
            continue;
        }
        if (ret) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto destroy_device;
        }

        cmd_ret = val_host_rmi_pdev_create_smc(args.pdev_ptr, args.params_ptr);
        ret = val_host_sro_complete(&cmd_ret);
        if (args.params_ptr_allocated != 0U)
        {
            val_host_mem_free((void *)args.params_ptr_allocated);
            args.params_ptr_allocated = 0U;
        }

        if (test_data[i].label == PDEV_TRACK_COARSE)
        {
            if ((RMI_STATUS(ret) != RMI_ERROR_TRACKING) ||
                (RMI_TRACKING_LEVEL(ret) != RMI_TRACKING_LEVEL_COARSE) ||
                (VAL_EXTRACT_BITS(ret, 16, 55) !=
                 (args.pdev_ptr >> PAGE_BITS_4K)))
            {
                LOG(ERROR, "PDEV tracking check returned 0x%lx, expected "
                    "TRACKING at 0x%lx\n", ret, args.pdev_ptr);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
                goto destroy_device;
            }

            if (pdev_track_cleanup() != VAL_SUCCESS)
            {
                LOG(ERROR, "PDEV tracking setup cleanup failed\n");
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
                goto destroy_device;
            }
            continue;
        }

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            if ((RMI_STATUS(ret) == RMI_SUCCESS) &&
                (args.pdev_ptr == c_args.pdev_ptr_valid))
            {
                pdev_created = true;
            }

            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_device;
        }

    }

    LOG(TEST, "\n\tPositive Observability Check\n");
    ret = val_host_rmi_pdev_create(c_args.pdev_ptr_valid, c_args.params_ptr_valid);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_PDEV_CREATE failed, ret=0x%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }
    pdev_created = true;

    /* Check PDEV state is PDEV_NEW */
    if (val_host_check_pdev_state(c_args.pdev_ptr_valid, RMI_PDEV_NEW))
    {
        LOG(ERROR, "PDEV state check failed.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_device;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    if (pdev_track_cleanup() != VAL_SUCCESS)
    {
        LOG(ERROR, "PDEV tracking setup cleanup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    if (args.params_ptr_allocated != 0U)
        val_host_mem_free((void *)args.params_ptr_allocated);

    c_args.pdev_dev_valid.pdev = c_args.pdev_ptr_valid;
    if (pdev_created &&
        val_host_pdev_teardown(&c_args.pdev_dev_valid, c_args.pdev_ptr_valid))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    if (!pdev_created && c_args.pdev_ptr_valid &&
        val_host_rmi_granule_range_undelegate(c_args.pdev_ptr_valid,
                                               c_args.pdev_ptr_valid + PAGE_SIZE))
    {
        LOG(ERROR, "PDEV undelegation failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
    }

    if (c_args.params_ptr_valid != 0U)
        val_host_mem_free((void *)c_args.params_ptr_valid);

exit:
    return;
}
