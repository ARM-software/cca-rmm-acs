/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_rmm_config_set_data.h"
#include "command_common_host.h"

static struct argument_store {
    val_host_rmi_rmm_config_ts *cfg;
    uint64_t cfg_ptr_valid;
    uint8_t rmi_granule_size;
    uint8_t tracking_region_size;
} c_args;

struct arguments {
    uint64_t cfg_ptr;
};

static uint64_t valid_input_args_prep_sequence(void)
{
    val_smc_param_ts cmd_ret;

    /*
     * RMI_RMM_CONFIG_GET and RMI_RMM_CONFIG_SET require an RMI-granule-aligned
     * buffer. Use 64KB alignment as it satisfies all valid RMI granule sizes.
     */
    c_args.cfg = val_host_mem_alloc(PAGE_SIZE_64K, PAGE_SIZE_64K);
    if (c_args.cfg == NULL)
    {
        LOG(ERROR, "Failed to allocate RMM config structure\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    c_args.cfg_ptr_valid = (uint64_t)c_args.cfg;
    val_memset(c_args.cfg, 0, PAGE_SIZE_64K);

    /*
     * Framework activates RMM before command tests run. Read the active
     * RMM configuration so cfg has valid supported values before checking that
     * RMI_RMM_CONFIG_SET fails in ACTIVE state.
     */
    cmd_ret = val_host_rmi_rmm_config_get(c_args.cfg_ptr_valid);
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_RMM_CONFIG_GET failed, ret=0x%x\n", cmd_ret.x0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    c_args.rmi_granule_size = c_args.cfg->rmi_granule_size;
    c_args.tracking_region_size = c_args.cfg->tracking_region_size;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case RMM_ACTIVE:
            args->cfg_ptr = c_args.cfg_ptr_valid;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_rmm_config_set_host(void)
{
    uint64_t i;
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

        cmd_ret = val_host_rmi_rmm_config_set(args.cfg_ptr);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "Test Failure!The ABI call returned: %x Expected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto free_cfg;
        }
    }

    val_memset(c_args.cfg, 0, sizeof(*c_args.cfg));
    cmd_ret = val_host_rmi_rmm_config_get(c_args.cfg_ptr_valid);
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_RMM_CONFIG_GET failed, ret=0x%x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto free_cfg;
    }

    if ((c_args.cfg->rmi_granule_size != c_args.rmi_granule_size) ||
        (c_args.cfg->tracking_region_size != c_args.tracking_region_size))
    {
        LOG(ERROR, "Failed RMI_RMM_CONFIG_SET changed the RMM configuration\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto free_cfg;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

free_cfg:
    val_host_mem_free(c_args.cfg);

exit:
    return;
}
