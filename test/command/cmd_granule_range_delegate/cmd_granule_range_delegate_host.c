/*
 * Copyright (c) 2023, 2025-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_rmi_sro.h"
#include "rmi_granule_range_delegate_data.h"
#include "command_common_host.h"

static struct argument_store {
    uint64_t base_valid;
    uint64_t top_valid;
    val_host_realm_ts state_realm;
    val_host_tracking_test_region_ts track_region;
} c_args;

struct arguments {
    uint64_t base;
    uint64_t top;
};

static uint64_t base_valid_prep_sequence(void)
{
    return g_undelegated_prep_sequence();
}

static uint64_t valid_input_args_prep_sequence(void)
{
    val_memset(&c_args, 0, sizeof(c_args));

    c_args.base_valid = base_valid_prep_sequence();
    if (c_args.base_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.top_valid = c_args.base_valid + PAGE_SIZE;

    return VAL_SUCCESS;
}

static uint64_t state_rd_prep_sequence(void)
{
    val_host_realm_params(&c_args.state_realm);
    if (val_host_realm_create_common(&c_args.state_realm))
        return VAL_TEST_PREP_SEQ_FAILED;

    return c_args.state_realm.rd;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case BASE_UNALIGNED:
            args->base = g_unaligned_prep_sequence(c_args.base_valid);
            args->top = c_args.base_valid + (2 * PAGE_SIZE);
            break;

        case TOP_UNALIGNED:
            args->base = c_args.base_valid;
            args->top = g_unaligned_prep_sequence(c_args.top_valid);
            break;

        case TOP_BOUND:
            args->base = c_args.base_valid;
            args->top = c_args.base_valid;
            break;

        case POPULATED:
            args->base = g_dev_mem_prep_sequence();
            args->top = args->base + PAGE_SIZE;
            break;

        case TRACKING_NONE:
            args->base = PLATFORM_TRACKING_TEST_REGION_BASE;
            args->top = args->base + PAGE_SIZE;
            return val_host_tracking_none_prep_sequence(&c_args.track_region);

        case STATE_RD:
            args->base = state_rd_prep_sequence();
            if (args->base == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->top = args->base + PAGE_SIZE;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}


void cmd_granule_range_delegate_host(void)
{
    uint64_t ret, i;
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

        ret = intent_to_seq(&test_data[i], &args);
        if (ret == VAL_SKIP_CHECK)
        {
            LOG(TEST, "Skipping Check %d\n", (int)(i + 1));
            continue;
        }
        if (ret != VAL_SUCCESS)
        {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        cmd_ret = val_host_rmi_granule_range_delegate_smc(args.base, args.top);
        if (test_data[i].label == TRACKING_NONE)
        {
            if ((RMI_STATUS(cmd_ret.x0) != RMI_ERROR_TRACKING) ||
                (VAL_EXTRACT_BITS(cmd_ret.x0, 16, 55) !=
                 (args.base >> PAGE_BITS_4K)))
            {
                LOG(ERROR, "Tracking check returned 0x%lx, expected "
                    "TRACKING at 0x%lx\n", cmd_ret.x0, args.base);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
                goto exit;
            }

            if (val_host_tracking_none_cleanup(&c_args.track_region) != VAL_SUCCESS)
            {
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
                goto exit;
            }
            continue;
        }

        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "Test Failure!The ABI call returned: %x Expected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);
    ret = val_host_rmi_granule_range_delegate(c_args.base_valid,
                                              c_args.top_valid);
    if (ret != 0)
    {
        LOG(ERROR, "Granule range delegate command failed, ret value is: %x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    if (val_host_tracking_none_cleanup(&c_args.track_region) != VAL_SUCCESS)
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
    return;
}
