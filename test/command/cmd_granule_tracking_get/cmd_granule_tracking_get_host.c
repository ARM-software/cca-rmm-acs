/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_granule_tracking_get_data.h"
#include "command_common_host.h"

static struct argument_store {
    uint64_t base_valid;
    uint64_t top_valid;
    uint64_t rmi_granule_size;
} c_args;

struct arguments {
    uint64_t base;
    uint64_t top;
};

static uint64_t valid_input_args_prep_sequence(void)
{
    uint64_t addr;
    val_smc_param_ts cmd_ret;
    val_host_rmi_rmm_config_ts *cfg;

    val_memset(&c_args, 0, sizeof(c_args));

    /* A 64KB-aligned buffer is aligned to every valid RMI Granule size. */
    cfg = val_host_mem_alloc(PAGE_SIZE_64K, PAGE_SIZE_64K);
    if (cfg == NULL)
    {
        LOG(ERROR, "Failed to allocate RMM config structure\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    val_memset(cfg, 0, PAGE_SIZE_64K);
    cmd_ret = val_host_rmi_rmm_config_get((uint64_t)cfg);
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_RMM_CONFIG_GET failed, ret=0x%x\n", cmd_ret.x0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    switch (cfg->rmi_granule_size)
    {
        case RMI_GRANULE_SIZE_4KB:
            c_args.rmi_granule_size = PAGE_SIZE_4K;
            break;
        case RMI_GRANULE_SIZE_16KB:
            c_args.rmi_granule_size = PAGE_SIZE_16K;
            break;
        case RMI_GRANULE_SIZE_64KB:
            c_args.rmi_granule_size = PAGE_SIZE_64K;
            break;
        default:
            LOG(ERROR, "Invalid RMI Granule size encoding: 0x%x\n",
                cfg->rmi_granule_size);
            return VAL_TEST_PREP_SEQ_FAILED;
    }

    val_host_mem_free(cfg);

    addr = (uint64_t)val_host_mem_alloc(c_args.rmi_granule_size,
                                        c_args.rmi_granule_size);
    if (addr == 0)
    {
        LOG(ERROR, "Failed to allocate tracking address\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    c_args.base_valid = addr;
    c_args.top_valid = addr + c_args.rmi_granule_size;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case BASE_UNALIGNED:
            args->base = g_unaligned_prep_sequence(c_args.base_valid);
            args->top = c_args.top_valid;
            break;

        case TOP_UNALIGNED:
            args->base = c_args.base_valid;
            args->top = g_unaligned_prep_sequence(c_args.top_valid);
            break;

        case BASE_BOUND:
            args->base = (1ULL << val_get_pa_range_supported()) +
                         c_args.rmi_granule_size;
            args->top = args->base + c_args.rmi_granule_size;
            break;

        case TOP_BOUND:
            args->base = c_args.base_valid;
            args->top = (1ULL << val_get_pa_range_supported()) +
                        c_args.rmi_granule_size;
            break;

        case BASE_TOP:
            args->base = c_args.base_valid;
            args->top = c_args.base_valid;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_granule_tracking_get_host(void)
{
    uint64_t i, expected_ret;
    val_smc_param_ts cmd_ret;
    struct arguments args;

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
              i + 1, test_data[i].msg, test_data[i].label);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto free_cfg;
        }

        cmd_ret = val_host_rmi_granule_tracking_get(args.base, args.top);

        expected_ret = PACK_CODE(test_data[i].status, test_data[i].index);
        if (cmd_ret.x0 != expected_ret) {
            LOG(ERROR, "Test Failure!The ABI call returned: %x Expected: %x\n",
                cmd_ret.x0, expected_ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto free_cfg;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);
    cmd_ret = val_host_rmi_granule_tracking_get(c_args.base_valid,
                                                c_args.top_valid);
    if ((cmd_ret.x0 != RMI_SUCCESS) || (cmd_ret.x3 != c_args.top_valid))
    {
        LOG(ERROR, "RMI_GRANULE_TRACKING_GET failed, ret=0x%x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto free_cfg;
    }

    if ((VAL_EXTRACT_BITS(cmd_ret.x1, 2, 63) != 0) ||
        (VAL_EXTRACT_BITS(cmd_ret.x2, 3, 63) != 0))
    {
        LOG(ERROR, "Reserved output bits are nonzero\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto free_cfg;
    }

    if (VAL_EXTRACT_BITS(cmd_ret.x1, 0, 1) != RMI_MEM_CATEGORY_CONVENTIONAL)
    {
        LOG(ERROR, "Unexpected memory category: 0x%x\n", cmd_ret.x1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto free_cfg;
    }

    if (VAL_EXTRACT_BITS(cmd_ret.x2, 0, 2) > RMI_TRACKING_COARSE)
    {
        LOG(ERROR, "Invalid tracking state: 0x%x\n", cmd_ret.x2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto free_cfg;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

free_cfg:
    if (c_args.base_valid != 0)
        val_host_mem_free((void *)c_args.base_valid);

exit:
    return;
}
