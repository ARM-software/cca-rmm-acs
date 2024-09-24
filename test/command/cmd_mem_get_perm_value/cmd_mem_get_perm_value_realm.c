/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "rsi_mem_get_perm_value_data.h"
#include "val_realm_framework.h"
#include "val_realm_planes.h"

#define PLANE_IDX_1                     1
#define PERM_IDX_0                      0
#define PLANE_IDX_OUT_OF_BOUND          2
#define PERM_IDX_OUT_OF_BOUND           15

static struct argument_store {
    uint64_t plane_index_valid;
    uint64_t perm_index_valid;
} c_args;

struct arguments {
    uint64_t plane_index;
    uint64_t perm_index;
};

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.plane_index_valid = PLANE_IDX_1;

    c_args.perm_index_valid = PERM_IDX_0;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case PLANE_INDEX_OUT_OF_BOUND:
            args->plane_index = PLANE_IDX_OUT_OF_BOUND;
            args->perm_index = c_args.perm_index_valid;
            break;

        case PERM_INDEX_OUT_OF_BOUND:
            args->plane_index = c_args.plane_index_valid;
            args->perm_index = PERM_IDX_OUT_OF_BOUND;
            break;

        default:
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_mem_get_perm_value_realm(void)
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
        LOG(TEST, "\n\tCheck %d : ", i + 1, 0);
        LOG(TEST, test_data[i].msg, 0, 0);
        LOG(TEST, "; intent id : 0x%x \n", test_data[i].label, 0);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        cmd_ret = val_realm_rsi_mem_get_perm_value(args.plane_index, args.perm_index);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);
    cmd_ret = val_realm_rsi_mem_get_perm_value(c_args.plane_index_valid, c_args.perm_index_valid);
    if (cmd_ret.x0 != 0 || cmd_ret.x1 != S2_AP_NO_ACCESS)
    {
        LOG(ERROR, "\n\t Command failed. %x\n", cmd_ret.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    val_realm_return_to_host();
}
