/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_psmmu_activate_data.h"
#include "command_common_host.h"
#include "psmmu_command_common_host.h"

static struct argument_store {
    uint64_t psmmu_ptr_valid;
    uint64_t params_ptr_valid;
    psmmu_cmd_context_ts ctx;
} c_args;

struct arguments {
    uint64_t psmmu_ptr;
    uint64_t params_ptr;
};

static uint64_t valid_input_args_prep_sequence(void)
{
    if (psmmu_cmd_context_init(&c_args.ctx) != VAL_SUCCESS) {
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    c_args.psmmu_ptr_valid = c_args.ctx.psmmu;
    c_args.params_ptr_valid = psmmu_cmd_params_prep_sequence(&c_args.ctx,
                                      PSMMU_CMD_PARAMS_VALID);
    if (c_args.params_ptr_valid == VAL_TEST_PREP_SEQ_FAILED) {
        psmmu_cmd_context_cleanup(&c_args.ctx);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;
    uint64_t ret;

    args->psmmu_ptr = c_args.psmmu_ptr_valid;
    args->params_ptr = c_args.params_ptr_valid;

    switch (label) {
    case psmmu_valid:
        args->psmmu_ptr = g_unaligned_prep_sequence(c_args.psmmu_ptr_valid);
        break;
    case params_align:
        args->params_ptr = g_unaligned_prep_sequence(c_args.params_ptr_valid);
        break;
    case params_pas:
        args->params_ptr = g_delegated_prep_sequence();
        if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED) {
            return VAL_TEST_PREP_SEQ_FAILED;
        }
        break;
    case irq_cfg:
        args->params_ptr = psmmu_cmd_params_prep_sequence(&c_args.ctx,
                                  PSMMU_CMD_PARAMS_IRQ_INVALID);
        break;
    case cmdq_sync_irq_wired:
        args->params_ptr = psmmu_cmd_params_prep_sequence(&c_args.ctx,
                                  PSMMU_CMD_PARAMS_CMDQ_SYNC_INVALID);
        break;
    case gerror_valid:
        args->params_ptr = psmmu_cmd_params_prep_sequence(&c_args.ctx,
                                  PSMMU_CMD_PARAMS_MSI_GERROR_INVALID);
        break;
    case eventq_valid:
        args->params_ptr = psmmu_cmd_params_prep_sequence(&c_args.ctx,
                                  PSMMU_CMD_PARAMS_MSI_EVENTQ_INVALID);
        break;
    case priq_valid:
        args->params_ptr = psmmu_cmd_params_prep_sequence(&c_args.ctx,
                                  PSMMU_CMD_PARAMS_MSI_PRIQ_INVALID);
        break;
    case ats_supp:
    case dpt:
        args->params_ptr = psmmu_cmd_params_prep_sequence(&c_args.ctx,
                                  PSMMU_CMD_PARAMS_ATS_TRUE);
        break;
    case pri_supp:
        args->params_ptr = psmmu_cmd_params_prep_sequence(&c_args.ctx,
                                  PSMMU_CMD_PARAMS_PRI_TRUE);
        break;
    case psmmu_state:
        ret = val_host_rmi_psmmu_activate(c_args.ctx.psmmu,
                                          c_args.params_ptr_valid);
        if ((ret == RMI_INCOMPLETE) || (ret != RMI_SUCCESS)) {
            return VAL_TEST_PREP_SEQ_FAILED;
        }
        c_args.ctx.activated = 1U;
        args->params_ptr = c_args.params_ptr_valid;
        break;
    default:
        LOG(ERROR, "Unknown intent label encountered\n");
        return VAL_ERROR;
    }

    if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED) {
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return VAL_SUCCESS;
}

static uint32_t check_activate_result(struct stimulus *stimulus, uint64_t ret)
{
    if ((stimulus->label == ats_supp) && (ret == RMI_ERROR_DPT)) {
        LOG(TEST, "\tATS is supported; skipping ats_supp unsupported check\n");
        return VAL_SKIP_CHECK;
    }

    if ((stimulus->label == dpt) && (ret == RMI_ERROR_INPUT)) {
        LOG(TEST, "\tATS is unsupported; skipping dpt missing check\n");
        return VAL_SKIP_CHECK;
    }

    if ((stimulus->label == pri_supp) && (ret == RMI_SUCCESS)) {
        LOG(TEST, "\tPRI is supported by this PSMMU, skipping unsupported check\n");
        return VAL_SKIP_CHECK;
    }

    if (ret != PACK_CODE(stimulus->status, stimulus->index)) {
        LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
            ret, PACK_CODE(stimulus->status, stimulus->index));
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_psmmu_activate_host(void)
{
    uint64_t i;
    uint64_t ret;
    uint32_t check_ret;
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

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto cleanup;
        }

        cmd_ret = val_host_rmi_psmmu_activate_smc(args.psmmu_ptr,
                                                   args.params_ptr);
        ret = val_host_sro_complete(&cmd_ret);
        check_ret = check_activate_result(&test_data[i], ret);
        if (check_ret == VAL_SKIP_CHECK) {
            if ((test_data[i].label == pri_supp) && (ret == RMI_SUCCESS)) {
                ret = val_host_rmi_psmmu_deactivate(c_args.ctx.psmmu);
                if ((ret == RMI_INCOMPLETE) || (ret != RMI_SUCCESS)) {
                    c_args.ctx.activated = 1;
                    val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
                    goto cleanup;
                }
                c_args.ctx.activated = 0;
            }
            continue;
        }
        if (check_ret != VAL_SUCCESS) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto cleanup;
        }
    }

    if (c_args.ctx.activated != 0U) {
        ret = val_host_rmi_psmmu_deactivate(c_args.ctx.psmmu);
        if ((ret == RMI_INCOMPLETE) || (ret != RMI_SUCCESS)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto cleanup;
        }
        c_args.ctx.activated = 0;
    }

    LOG(TEST, "\n\tPositive Observability Check\n");
    ret = val_host_rmi_psmmu_activate(c_args.ctx.psmmu,
                                      c_args.params_ptr_valid);
    if ((ret == RMI_INCOMPLETE) || (ret != RMI_SUCCESS)) {
        LOG(ERROR, "RMI_PSMMU_ACTIVATE command failed. %x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto cleanup;
    }
    c_args.ctx.activated = 1;

    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    psmmu_cmd_context_cleanup(&c_args.ctx);
}
