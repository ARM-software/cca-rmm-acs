/*
 * Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "rsi_mem_set_perm_value_data.h"
#include "val_realm_framework.h"
#include "val_realm_planes.h"

#define PLANE_INDEX_1                   1
#define PERM_INDEX_1                    1
#define PERM_INDEX_2                    2
#define PLANE_IDX_OUT_OF_BOUND          2
#define PERMISSION_INDEX_OUT_OF_BOUND   15
#define OVERLAY_UNSUPPORTED             0x10UL

static struct argument_store {
    uint64_t plane_index_valid;
    uint64_t perm_index_valid;
    uint64_t value_valid;
} c_args;

struct arguments {
    uint64_t plane_index;
    uint64_t perm_index;
    uint64_t value;
};

static uint64_t perm_index_locked_prep_sequence(void)
{
    val_smc_param_ts cmd_ret;
    uint64_t ipa_base, ipa_top, cookie;

    cmd_ret = val_realm_rsi_mem_set_perm_value(PLANE_INDEX_1, PERM_INDEX_2, S2_AP_RW_upX);

    if (cmd_ret.x0)
    {
        LOG(ERROR, "RSI_MEM_SET_PERM_VALUE failed with ret = %d\n", cmd_ret.x0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    ipa_base = 0x0;
    ipa_top = 0x1000;
    cookie = 0;

    while (ipa_base != ipa_top)
    {
        cmd_ret = val_realm_rsi_mem_set_perm_index(ipa_base, ipa_top, PERM_INDEX_2, cookie);

        if (cmd_ret.x0 || cmd_ret.x2 == RSI_REJECT)
        {
            LOG(ERROR, "RSI_MEM_SET_PERM_INDEX failed with ret = %d\n", cmd_ret.x0);
            return VAL_TEST_PREP_SEQ_FAILED;
        }

        ipa_base = cmd_ret.x1;
        cookie = cmd_ret.x3;
    }

    return PERM_INDEX_2;
}


static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.plane_index_valid = PLANE_INDEX_1 ;

    c_args.perm_index_valid = PERM_INDEX_1 ;

    c_args.value_valid = S2_AP_RW_upX ;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case PLANE_INDEX_0:
            args->plane_index = PLANE_INDEX_0;
            args->perm_index = c_args.perm_index_valid;
            args->value = c_args.value_valid;
            break;

        case PLANE_INDEX_OUT_OF_BOUND:
            args->plane_index = PLANE_IDX_OUT_OF_BOUND;
            args->perm_index = c_args.perm_index_valid;
            args->value = c_args.value_valid;
            break;

        case PERM_INDEX_OUT_OF_BOUND:
            args->plane_index = c_args.plane_index_valid;
            args->perm_index = PERMISSION_INDEX_OUT_OF_BOUND;
            args->value = c_args.value_valid;
            break;

        case PERM_INDEX_LOCKED:
            args->plane_index = c_args.plane_index_valid;
            args->perm_index = perm_index_locked_prep_sequence();
            if (args->perm_index == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->value = c_args.value_valid;
            break;

        case PERM_VALUE_UNSUPPORTED:
            args->plane_index = c_args.plane_index_valid;
            args->perm_index = c_args.perm_index_valid;
            args->value = OVERLAY_UNSUPPORTED;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_mem_set_perm_value_realm(void)
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
            goto exit;
        }

        cmd_ret = val_realm_rsi_mem_set_perm_value(args.plane_index, args.perm_index, args.value);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "Test Failure!The ABI call returned: %xExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }


    LOG(TEST, "Check %2d : Positive Observability\n", ++i);
    cmd_ret = val_realm_rsi_mem_set_perm_value(c_args.plane_index_valid,
                                         c_args.perm_index_valid, c_args.value_valid);
    if (cmd_ret.x0 != 0)
    {
        LOG(ERROR, " Command failed. %x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    /* Check if permission overlay table is updated with the new value */
    cmd_ret = val_realm_rsi_mem_get_perm_value(c_args.plane_index_valid, c_args.perm_index_valid);
    if (cmd_ret.x0 || cmd_ret.x1 != S2_AP_RW_upX)
    {
        LOG(ERROR, " Unexpected output for RSI_MEM_GET_PERM_VALUE, ret %%d, val = %x\n",
                                                                                     cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    val_realm_return_to_host();
}
