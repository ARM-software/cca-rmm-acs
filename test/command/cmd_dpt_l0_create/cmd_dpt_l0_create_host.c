/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_smc.h"
#include "rmi_dpt_l0_create_data.h"
#include "psmmu_command_common_host.h"

static struct argument_store {
    psmmu_cmd_context_ts ctx;
} c_args;

static uint64_t valid_input_args_prep_sequence(void)
{
    if (psmmu_cmd_context_init(&c_args.ctx) != VAL_SUCCESS) {
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data)
{
    uint64_t ret;
    val_smc_param_ts cmd_ret;

    switch (test_data->label) {
    case feat:
        if (psmmu_cmd_ats_supported() != 0U) {
            return VAL_SKIP_CHECK;
        }
        break;
    case l0dpt_state:
        cmd_ret = val_host_rmi_dpt_l0_create();
        ret = val_host_sro_complete(&cmd_ret);
        if (psmmu_cmd_dpt_not_supported(ret)) {
            return VAL_SKIP_CHECK;
        }
        if ((ret == RMI_INCOMPLETE) || (ret != RMI_SUCCESS)) {
            return VAL_TEST_PREP_SEQ_FAILED;
        }
        c_args.ctx.l0_created = 1U;
        break;
    default:
        LOG(ERROR, "Unknown intent label encountered\n");
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_dpt_l0_create_host(void)
{
    uint64_t i;
    uint64_t ret;
    uint64_t prep_ret;
    val_smc_param_ts cmd_ret;

    if (psmmu_cmd_ats_supported() == 0U) {
        LOG(TEST, "\n\tATS feature not supported; validating feat failure only\n");
        cmd_ret = val_host_rmi_dpt_l0_create();
        ret = val_host_sro_complete(&cmd_ret);
        if (ret != PACK_CODE(RMI_ERROR_NOT_SUPPORTED, 0)) {
            LOG(ERROR, "RMI_DPT_L0_CREATE command failed. %x\n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            return;
        }
        val_set_status(RESULT_PASS(VAL_SUCCESS));
        return;
    }

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++) {
        LOG(TEST, "\n\tCheck %d : ", i + 1);
        LOG(TEST, "%s", test_data[i].msg);
        LOG(TEST, "; intent id : 0x%x \n", test_data[i].label);

        prep_ret = intent_to_seq(&test_data[i]);
        if (prep_ret == VAL_SKIP_CHECK) {
            LOG(TEST, "\tSkipping condition that requires ATS to be unsupported\n");
            continue;
        }
        if (prep_ret != VAL_SUCCESS) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto cleanup;
        }

        cmd_ret = val_host_rmi_dpt_l0_create();
        ret = val_host_sro_complete(&cmd_ret);
        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto cleanup;
        }
    }

    cmd_ret = val_host_rmi_dpt_l0_destroy();
    ret = val_host_sro_complete(&cmd_ret);
    if ((ret == RMI_INCOMPLETE) || (ret != RMI_SUCCESS)) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto cleanup;
    }
    c_args.ctx.l0_created = 0U;

    LOG(TEST, "\n\tPositive Observability Check\n");
    cmd_ret = val_host_rmi_dpt_l0_create();
    ret = val_host_sro_complete(&cmd_ret);
    if (psmmu_cmd_dpt_not_supported(ret)) {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto cleanup;
    }
    if ((ret == RMI_INCOMPLETE) || (ret != RMI_SUCCESS)) {
        LOG(ERROR, "RMI_DPT_L0_CREATE command failed. %x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto cleanup;
    }
    c_args.ctx.l0_created = 1U;

    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    psmmu_cmd_context_cleanup(&c_args.ctx);
}
