/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "rsi_mem_set_perm_index_data.h"
#include "val_realm_framework.h"
#include "val_realm_planes.h"

#define PLANE_IDX_1             1
#define PERM_IDX_1              1
#define PERM_IDX_OUT_OF_BOUND  15
#define VALID_COOKIE            0
#define INVALID_COOKIE          4

/* Allocate base IPA which is 4KB aligned and Protected */
__attribute__((aligned (PAGE_SIZE))) static uint8_t base_gran[PAGE_SIZE];

static struct argument_store {
    uint64_t base_valid;
    uint64_t top_valid;
    uint64_t perm_index_valid;
    uint64_t cookie_valid;
} c_args;

struct arguments {
    uint64_t base;
    uint64_t top;
    uint64_t perm_index;
    uint64_t cookie;
};

static uint64_t perm_index_valid_prep_sequence(void)
{
    val_smc_param_ts cmd_ret;

    cmd_ret = val_realm_rsi_mem_set_perm_value(PLANE_IDX_1, PERM_IDX_1, S2_AP_RW_upX);

    if (cmd_ret.x0)
    {
        LOG(ERROR, "RSI_MEM_SET_PERM_VALUE failed with ret = %d\n", cmd_ret.x0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return PERM_IDX_1;
}

static uint64_t region_unprotected_prep_sequence(void)
{
    uint64_t ipa_width;

    ipa_width = val_realm_get_ipa_width();
    return (1ULL << (ipa_width - 1)) + PAGE_SIZE;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.base_valid = (uint64_t)base_gran;

    c_args.top_valid = (uint64_t)base_gran + PAGE_SIZE;

    c_args.perm_index_valid = perm_index_valid_prep_sequence();

    c_args.cookie_valid = VALID_COOKIE;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case BASE_UNALIGNED:
            args->base = c_args.base_valid - PAGE_SIZE / 2;
            args->top = c_args.top_valid;
            args->perm_index = c_args.perm_index_valid;
            args->cookie = c_args.cookie_valid;
            break;

        case TOP_UNALIGNED:
            args->base = c_args.base_valid;
            args->top = c_args.top_valid - PAGE_SIZE / 2;
            args->perm_index = c_args.perm_index_valid;
            args->cookie = c_args.cookie_valid;
            break;

        case SIZE_INVALID:
            args->base = c_args.base_valid;
            args->top = c_args.base_valid;
            args->perm_index = c_args.perm_index_valid;
            args->cookie = c_args.cookie_valid;
            break;

        case REGION_UNPROTECTED:
            args->base = c_args.base_valid;
            args->top = region_unprotected_prep_sequence();
            args->perm_index = c_args.perm_index_valid;
            args->cookie = c_args.cookie_valid;
            break;

        case PERM_INDEX_OUT_OF_BOUND:
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            args->perm_index = PERM_IDX_OUT_OF_BOUND;
            args->cookie = c_args.cookie_valid;
            break;

        case COOKIE_INVALID:
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            args->perm_index = c_args.perm_index_valid;
            args->cookie = INVALID_COOKIE;
            break;

        default:
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_mem_set_perm_index_realm(void)
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

        cmd_ret = val_realm_rsi_mem_set_perm_index(args.base, args.top,
                                                         args.perm_index, args.cookie);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);
    cmd_ret = val_realm_rsi_mem_set_perm_index(c_args.base_valid, c_args.top_valid,
                                               c_args.perm_index_valid, c_args.cookie_valid);
    if (cmd_ret.x0 != 0 || cmd_ret.x1 != c_args.top_valid || cmd_ret.x2 != RSI_ACCEPT)
    {
        LOG(ERROR, "\n\t Command failed. %x\n", cmd_ret.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    /* Check for host rejection of S2AP change request */
    cmd_ret = val_realm_rsi_mem_set_perm_index(c_args.base_valid, c_args.top_valid,
                                               c_args.perm_index_valid, c_args.cookie_valid);
    if (cmd_ret.x2 != RSI_REJECT)
    {
        LOG(ERROR, "\n\tInvalid response value for RSI_MEM_SET_PERM_INDEX %x\n", cmd_ret.x2, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    val_realm_return_to_host();
}
