/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_dpt_l1_create_data.h"
#include "psmmu_command_common_host.h"

static struct argument_store {
    uint64_t addr_valid;
    psmmu_cmd_context_ts ctx;
} c_args;

struct arguments {
    uint64_t addr;
};

static uint64_t valid_input_args_prep_sequence(void)
{
    if (psmmu_cmd_context_init(&c_args.ctx) != VAL_SUCCESS) {
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    c_args.addr_valid = c_args.ctx.l1_addr;
    return VAL_SUCCESS;
}

static uint64_t ensure_l0(void)
{
    uint64_t ret;
    val_smc_param_ts cmd_ret;

    if (c_args.ctx.l0_created != 0U) {
        return VAL_SUCCESS;
    }

    cmd_ret = val_host_rmi_dpt_l0_create();
    ret = val_host_sro_complete(&cmd_ret);
    if (psmmu_cmd_dpt_not_supported(ret)) {
        return VAL_SKIP_CHECK;
    }
    if ((ret == RMI_INCOMPLETE) || (ret != RMI_SUCCESS)) {
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    c_args.ctx.l0_created = 1U;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;
    uint64_t ret;
    val_smc_param_ts cmd_ret;

    args->addr = c_args.addr_valid;

    switch (label) {
    case l0dpt_state:
        break;
    case addr_bound:
        ret = ensure_l0();
        if (ret != VAL_SUCCESS) {
            return ret;
        }
        args->addr = psmmu_cmd_dpt_addr_out_of_range();
        break;
    case addr_align:
        ret = ensure_l0();
        if (ret != VAL_SUCCESS) {
            return ret;
        }
        args->addr = psmmu_cmd_dpt_addr_unaligned(&c_args.ctx);
        break;
    case entry_state:
        ret = ensure_l0();
        if (ret != VAL_SUCCESS) {
            return ret;
        }
        cmd_ret = val_host_rmi_dpt_l1_create(c_args.addr_valid);
        ret = val_host_sro_complete(&cmd_ret);
        if ((ret == RMI_INCOMPLETE) || (ret != RMI_SUCCESS)) {
            return VAL_TEST_PREP_SEQ_FAILED;
        }
        c_args.ctx.l1_created = 1U;
        c_args.ctx.l1_addr = c_args.addr_valid;
        break;
    default:
        LOG(ERROR, "Unknown intent label encountered\n");
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_dpt_l1_create_host(void)
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
            val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
            goto cleanup;
        }
        if (prep_ret != VAL_SUCCESS) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto cleanup;
        }

        cmd_ret = val_host_rmi_dpt_l1_create(args.addr);
        ret = val_host_sro_complete(&cmd_ret);
        if (psmmu_cmd_dpt_not_supported(ret)) {
            val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
            goto cleanup;
        }
        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto cleanup;
        }
    }

    if (c_args.ctx.l1_created != 0U) {
        cmd_ret = val_host_rmi_dpt_l1_destroy(c_args.addr_valid);
        ret = val_host_sro_complete(&cmd_ret);
        if ((ret == RMI_INCOMPLETE) || (ret != RMI_SUCCESS)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto cleanup;
        }
        c_args.ctx.l1_created = 0U;
    }

    if (ensure_l0() != VAL_SUCCESS) {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto cleanup;
    }

    LOG(TEST, "\n\tPositive Observability Check\n");
    cmd_ret = val_host_rmi_dpt_l1_create(c_args.addr_valid);
    ret = val_host_sro_complete(&cmd_ret);
    if ((ret == RMI_INCOMPLETE) || (ret != RMI_SUCCESS)) {
        LOG(ERROR, "RMI_DPT_L1_CREATE command failed. %x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto cleanup;
    }
    c_args.ctx.l1_created = 1U;
    c_args.ctx.l1_addr = c_args.addr_valid;

    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    psmmu_cmd_context_cleanup(&c_args.ctx);
}
