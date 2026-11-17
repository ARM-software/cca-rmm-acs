/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_psmmu_deactivate_data.h"
#include "command_common_host.h"
#include "psmmu_command_common_host.h"

static struct argument_store {
    uint64_t psmmu_ptr_valid;
    psmmu_cmd_context_ts ctx;
} c_args;

struct arguments {
    uint64_t psmmu_ptr;
};

static uint64_t valid_input_args_prep_sequence(void)
{
    if (psmmu_cmd_context_init(&c_args.ctx) != VAL_SUCCESS) {
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    c_args.psmmu_ptr_valid = c_args.ctx.psmmu;
    return VAL_SUCCESS;
}

static uint64_t activate_for_deactivate(void)
{
    uint64_t params_ptr;
    uint64_t ret;

    if (c_args.ctx.activated != 0U) {
        return VAL_SUCCESS;
    }

    params_ptr = psmmu_cmd_params_prep_sequence(&c_args.ctx,
                                               PSMMU_CMD_PARAMS_VALID);
    if (params_ptr == VAL_TEST_PREP_SEQ_FAILED) {
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    ret = val_host_rmi_psmmu_activate(c_args.ctx.psmmu, params_ptr);
    if ((ret == RMI_INCOMPLETE) || (ret != RMI_SUCCESS)) {
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    c_args.ctx.activated = 1U;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;
    uint64_t ret;

    args->psmmu_ptr = c_args.psmmu_ptr_valid;

    switch (label) {
    case psmmu_valid:
        args->psmmu_ptr = g_unaligned_prep_sequence(c_args.psmmu_ptr_valid);
        break;
    case psmmu_state:
        if (c_args.ctx.activated != 0U) {
            ret = val_host_rmi_psmmu_deactivate(c_args.ctx.psmmu);
            if ((ret == RMI_INCOMPLETE) || (ret != RMI_SUCCESS)) {
                return VAL_TEST_PREP_SEQ_FAILED;
            }
            c_args.ctx.activated = 0U;
        }
        break;
    case psmmu_live:
        if (activate_for_deactivate() != VAL_SUCCESS) {
            return VAL_TEST_PREP_SEQ_FAILED;
        }
        ret = val_host_rmi_psmmu_st_l2_create(c_args.ctx.psmmu,
                                              c_args.ctx.sid);
        if ((ret == RMI_ERROR_NOT_SUPPORTED) || (ret == RMI_ERROR_INPUT)) {
            return VAL_SKIP_CHECK;
        }
        if ((ret == RMI_INCOMPLETE) || (ret != RMI_SUCCESS)) {
            return VAL_TEST_PREP_SEQ_FAILED;
        }
        c_args.ctx.l2_created = 1U;
        break;
    default:
        LOG(ERROR, "Unknown intent label encountered\n");
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_psmmu_deactivate_host(void)
{
    uint64_t i;
    uint64_t ret;
    uint64_t prep_ret;
    val_smc_param_ts cmd_ret;
    struct arguments args;

    if (!val_host_rmm_supports_da()) {
        LOG(ALWAYS, "DA feature not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
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

        prep_ret = intent_to_seq(&test_data[i], &args);
        if (prep_ret == VAL_SKIP_CHECK) {
            LOG(TEST, "\tSkipping platform-specific live ST check\n");
            continue;
        }
        if (prep_ret != VAL_SUCCESS) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto cleanup;
        }

        cmd_ret = val_host_rmi_psmmu_deactivate_smc(args.psmmu_ptr);
        ret = val_host_sro_complete(&cmd_ret);
        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto cleanup;
        }
    }

    if (c_args.ctx.l2_created != 0U) {
        ret = val_host_rmi_psmmu_st_l2_destroy(c_args.ctx.psmmu,
                                               c_args.ctx.sid);
        if ((ret == RMI_INCOMPLETE) || (ret != RMI_SUCCESS)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto cleanup;
        }
        c_args.ctx.l2_created = 0U;
    }

    if (activate_for_deactivate() != VAL_SUCCESS) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto cleanup;
    }

    LOG(TEST, "\n\tPositive Observability Check\n");
    ret = val_host_rmi_psmmu_deactivate(c_args.ctx.psmmu);
    if ((ret == RMI_INCOMPLETE) || (ret != RMI_SUCCESS)) {
        LOG(ERROR, "RMI_PSMMU_DEACTIVATE command failed. %x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto cleanup;
    }
    c_args.ctx.activated = 0U;

    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    psmmu_cmd_context_cleanup(&c_args.ctx);
}
