/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_psmmu_info_data.h"
#include "command_common_host.h"
#include "psmmu_command_common_host.h"

static struct argument_store {
    uint64_t psmmu_ptr_valid;
    uint64_t info_ptr_valid;
    val_host_psmmu_info_ts *info;
} c_args;

struct arguments {
    uint64_t psmmu_ptr;
    uint64_t info_ptr;
};

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.psmmu_ptr_valid = PSMMU_FVP_BASE_ADDR;
    c_args.info = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (c_args.info == NULL) {
        LOG(ERROR, "Failed to allocate PSMMU info buffer.\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    val_memset(c_args.info, 0, PAGE_SIZE_4K);
    c_args.info_ptr_valid = (uint64_t)c_args.info;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    args->psmmu_ptr = c_args.psmmu_ptr_valid;
    args->info_ptr = c_args.info_ptr_valid;

    switch (label) {
    case psmmu_valid:
        args->psmmu_ptr = g_unaligned_prep_sequence(c_args.psmmu_ptr_valid);
        break;
    case info_align:
        args->info_ptr = g_unaligned_prep_sequence(c_args.info_ptr_valid);
        break;
    case info_pas:
        args->info_ptr = g_delegated_prep_sequence();
        if (args->info_ptr == VAL_TEST_PREP_SEQ_FAILED) {
            return VAL_TEST_PREP_SEQ_FAILED;
        }
        break;
    default:
        LOG(ERROR, "Unknown intent label encountered\n");
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_psmmu_info_host(void)
{
    uint64_t i;
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

        cmd_ret = val_host_rmi_psmmu_info(args.psmmu_ptr, args.info_ptr);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto cleanup;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n");
    cmd_ret = val_host_rmi_psmmu_info(c_args.psmmu_ptr_valid,
                                      c_args.info_ptr_valid);
    if (cmd_ret.x0 != RMI_SUCCESS) {
        LOG(ERROR, "RMI_PSMMU_INFO command failed. %x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto cleanup;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    if (c_args.info != NULL) {
        val_host_mem_free(c_args.info);
        c_args.info = NULL;
    }
}
