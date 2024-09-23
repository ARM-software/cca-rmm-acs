/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "rsi_plane_enter_data.h"
#include "val_realm_framework.h"
#include "val_realm_planes.h"

#define PLANE_IDX_0                 0
#define PLANE_IDX_1                 1
#define PLANE_IDX_OUT_OF_BOUND      2

__attribute__((aligned (PAGE_SIZE))) val_realm_rsi_plane_run_ts run_ptr = {0};

static struct argument_store {
    uint64_t plane_idx_valid;
    uint64_t run_ptr_valid;
} c_args;

struct arguments {
    uint64_t plane_idx;
    uint64_t run_ptr;
};

static uint64_t address_unprotected_prep_sequence(void)
{
    uint64_t ipa_width;

    ipa_width = val_realm_get_ipa_width();
    return 1ULL << (ipa_width - 1);
}

static uint64_t run_ptr_valid_prep_sequence(void)
{
    uint64_t p1_ipa_base, p1_ipa_top;

    /* Configure Permissions for Plane 1 image */
    p1_ipa_base = VAL_PLANE1_IMAGE_BASE_IPA;
    p1_ipa_top = p1_ipa_base + PLATFORM_REALM_IMAGE_SIZE;

    if (val_realm_plane_perm_init(PLANE_1_INDEX, PLANE_1_PERMISSION_INDEX, p1_ipa_base,
                                                                             p1_ipa_top))
    {
        LOG(ERROR, "Secondary plane permission initialization failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    run_ptr.enter.pc =  VAL_PLANE1_IMAGE_BASE_IPA;

    return (uint64_t)&run_ptr;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.plane_idx_valid = PLANE_IDX_1;

    c_args.run_ptr_valid = run_ptr_valid_prep_sequence();
    if (c_args.run_ptr_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_ERROR;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case PLANE_INDEX_0:
            args->plane_idx = PLANE_IDX_0;
            args->run_ptr = c_args.run_ptr_valid;
            break;

        case PLANE_INDEX_OUT_OF_BOUND:
            args->plane_idx = PLANE_IDX_OUT_OF_BOUND;
            args->run_ptr = c_args.run_ptr_valid;
            break;

        case RUN_PTR_UNALIGNED:
            args->plane_idx = c_args.plane_idx_valid;
            args->run_ptr = c_args.run_ptr_valid - PAGE_SIZE / 2;
            break;

        case RUN_PTR_UNPROTECTED:
            args->plane_idx = c_args.plane_idx_valid;
            args->run_ptr = address_unprotected_prep_sequence();
            break;

        default:
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static void p0_payload(void)
{
    uint64_t i;
    val_smc_param_ts cmd_ret;
    struct arguments args;
    val_realm_rsi_plane_run_ts *ptr;

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

        cmd_ret = val_realm_rsi_plane_enter(args.plane_idx, args.run_ptr);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);

    cmd_ret = val_realm_rsi_plane_enter(c_args.plane_idx_valid, c_args.run_ptr_valid);
    if (cmd_ret.x0 != 0)
    {
        LOG(ERROR, "\n\t PLANE_ENTER Command failed. %x\n", cmd_ret.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    ptr = (val_realm_rsi_plane_run_ts *) c_args.run_ptr_valid;
    if (ptr->exit.reason != RSI_EXIT_SYNC)
    {
        LOG(ERROR, "\n\t PLANE_ENTER command failed. %x\n", cmd_ret.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    val_realm_return_to_host();
}

static void p1_payload(void)
{
    val_realm_return_to_p0();
}

void cmd_plane_enter_realm(void)
{
    if (val_realm_in_p0())
        p0_payload();
    else
        p1_payload();
}

