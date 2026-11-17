/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_rmm_config_get_data.h"
#include "command_common_host.h"

static struct argument_store {
    uint64_t cfg_ptr_valid;
} c_args;

struct arguments {
    uint64_t cfg_ptr;
};

static uint64_t valid_input_args_prep_sequence(void)
{
    val_host_rmi_rmm_config_ts *cfg = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    if (cfg == NULL)
    {
        LOG(ERROR, "Failed to allocate RMM config structure\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    c_args.cfg_ptr_valid = (uint64_t)cfg;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case CFG_UNALIGNED:
            args->cfg_ptr = g_unaligned_prep_sequence(c_args.cfg_ptr_valid);
            break;

        case CFG_PAS_REALM:
            args->cfg_ptr = g_delegated_prep_sequence();
            if (args->cfg_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case CFG_PAS_SECURE:
            args->cfg_ptr = g_secure_prep_sequence();
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_rmm_config_get_host(void)
{
    uint64_t i;
    val_smc_param_ts cmd_ret;
    struct arguments args;
    val_host_rmi_rmm_config_ts *cfg;

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

        cmd_ret = val_host_rmi_rmm_config_get(args.cfg_ptr);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "Test Failure!The ABI call returned: %x Expected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto free_cfg;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);
    cfg = (val_host_rmi_rmm_config_ts *)c_args.cfg_ptr_valid;
    val_memset(cfg, 0, sizeof(*cfg));

    cmd_ret = val_host_rmi_rmm_config_get(c_args.cfg_ptr_valid);
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_RMM_CONFIG_GET failed, ret=0x%x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto free_cfg;
    }

    if (!(((cfg->rmi_granule_size == RMI_GRANULE_SIZE_4KB) &&
           (cfg->tracking_region_size <= 1U)) ||
          (((cfg->rmi_granule_size == RMI_GRANULE_SIZE_16KB) ||
            (cfg->rmi_granule_size == RMI_GRANULE_SIZE_64KB)) &&
           (cfg->tracking_region_size <= 1U))))
    {
        LOG(ERROR, "Invalid RMM configuration: granule=0x%x, tracking=0x%x\n",
            cfg->rmi_granule_size, cfg->tracking_region_size);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto free_cfg;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

free_cfg:
    val_host_mem_free((void *)c_args.cfg_ptr_valid);

exit:
    return;
}
