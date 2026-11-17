/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_rmi_sro.h"
#include "rmi_gpt_l1_destroy_data.h"
#include "command_common_host.h"

static struct argument_store {
    uint64_t addr_valid;
    uint64_t alloc_valid;
    uint64_t l0gpt_size;
    bool entry_state_created;
    bool granule_delegated;
} c_args;

struct arguments {
    uint64_t addr;
};

static uint64_t valid_input_args_prep_sequence(void)
{
    uint64_t feature, l0gptsz;

    val_memset(&c_args, 0, sizeof(c_args));
    if (val_host_rmi_features(RMI_FEATURE_REGISTER_1_INDEX, &feature) != RMI_SUCCESS)
        return VAL_TEST_PREP_SEQ_FAILED;

    l0gptsz = VAL_EXTRACT_BITS(feature, RMI_FEATURE_REGISTER_1_L0GPTSZ_START,
                              RMI_FEATURE_REGISTER_1_L0GPTSZ_END);
    c_args.l0gpt_size = 1ULL << (RMI_L0GPTSZ_BASE_SHIFT + l0gptsz);
    c_args.alloc_valid = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (c_args.alloc_valid == 0)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.addr_valid = ADDR_ALIGN_DOWN(c_args.alloc_valid, c_args.l0gpt_size);
    return VAL_SUCCESS;
}

static uint64_t gpt_l1_homo_prep_sequence(void)
{
    if (val_host_rmi_gpt_l1_create(c_args.addr_valid) != RMI_SUCCESS)
        return VAL_TEST_PREP_SEQ_FAILED;
    c_args.entry_state_created = true;

    if (val_host_rmi_granule_range_delegate(c_args.alloc_valid,
                                            c_args.alloc_valid + PAGE_SIZE) != RMI_SUCCESS)
        return VAL_TEST_PREP_SEQ_FAILED;
    c_args.granule_delegated = true;

    return VAL_SUCCESS;
}

static uint64_t homo_granule_undelegate_sequence(void)
{
    if (!c_args.granule_delegated)
        return VAL_SUCCESS;

    if (val_host_rmi_granule_range_undelegate(c_args.alloc_valid,
                                               c_args.alloc_valid + PAGE_SIZE) != RMI_SUCCESS)
        return VAL_ERROR;
    c_args.granule_delegated = false;
    /* The range helper releases the tracked allocation on undelegation. */
    c_args.alloc_valid = 0;

    return VAL_SUCCESS;
}

static uint64_t entry_state_cleanup_sequence(void)
{
    if (homo_granule_undelegate_sequence() != VAL_SUCCESS)
        return VAL_ERROR;

    if (c_args.entry_state_created)
    {
        if (val_host_rmi_gpt_l1_destroy(c_args.addr_valid) != RMI_SUCCESS)
            return VAL_ERROR;
        c_args.entry_state_created = false;
    }

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case ADDR_BOUND:
            args->addr = ADDR_ALIGN_DOWN(g_outside_of_permitted_pa_prep_sequence(),
                                         c_args.l0gpt_size);
            break;

        case ADDR_UNALIGNED:
            args->addr = g_unaligned_prep_sequence(c_args.addr_valid);
            break;

        case ENTRY_STATE:
            args->addr = c_args.addr_valid;
            break;

        case GPT_L1_HOMO:
            args->addr = c_args.addr_valid;
            if (gpt_l1_homo_prep_sequence() != VAL_SUCCESS)
                return VAL_TEST_PREP_SEQ_FAILED;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_gpt_l1_destroy_host(void)
{
    uint64_t i, ret, expected_ret;
    struct arguments args;

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
            i + 1, test_data[i].msg, test_data[i].label);

        if (intent_to_seq(&test_data[i], &args))
        {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto free_addr;
        }

        ret = val_host_rmi_gpt_l1_destroy_smc(args.addr).x0;
        expected_ret = PACK_CODE(test_data[i].status, test_data[i].index);
        if (ret != expected_ret)
        {
            LOG(ERROR, "RMI_GPT_L1_DESTROY returned 0x%x, expected 0x%x\n",
                ret, expected_ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto free_addr;
        }
    }

    if (homo_granule_undelegate_sequence() != VAL_SUCCESS)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto free_addr;
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);
    ret = val_host_rmi_gpt_l1_destroy(c_args.addr_valid);
    if (ret != RMI_SUCCESS)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto free_addr;
    }
    c_args.entry_state_created = false;

    LOG(TEST, "Check %2d : Negative Observability\n", ++i);
    if (val_host_rmi_gpt_l1_destroy(c_args.addr_valid) != RMI_ERROR_INPUT)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto free_addr;
    }
    val_set_status(RESULT_PASS(VAL_SUCCESS));

free_addr:
    if (entry_state_cleanup_sequence() != VAL_SUCCESS)
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
    if ((c_args.alloc_valid != 0) && !c_args.granule_delegated)
        val_host_mem_free((void *)c_args.alloc_valid);

exit:
    return;
}
