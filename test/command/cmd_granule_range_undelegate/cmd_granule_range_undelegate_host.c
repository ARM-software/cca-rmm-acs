/*
 * Copyright (c) 2023, 2025-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_rmi_sro.h"
#include "rmi_granule_range_undelegate_data.h"
#include "command_common_host.h"

#define IPA_ADDR_CONTENT (6UL * PAGE_SIZE)
#define CONTENT_PATTERN 0xA5

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
    uint64_t base;

    base = g_undelegated_prep_sequence();
    if (base == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    if (val_host_rmi_granule_range_delegate(base, base + PAGE_SIZE))
        return VAL_TEST_PREP_SEQ_FAILED;

    return base;
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

static uint64_t content_changed_check(void)
{
    val_host_rmi_rtt_unmap_flags_ts unmap_flags = {0};
    uint64_t data, src, flags = 0;

    if (c_args.state_realm.rd == 0 ||
        create_mapping(IPA_ADDR_CONTENT, true, c_args.state_realm.rd)) {
        LOG(ERROR, "Could not prepare the DATA mapping\n");
        return VAL_ERROR;
    }

    data = g_delegated_prep_sequence();
    src = g_undelegated_prep_sequence();
    if (data == VAL_ERROR || src == VAL_ERROR) {
        LOG(ERROR, "Could not allocate content-check Granules\n");
        return VAL_ERROR;
    }

    val_memset((void *)src, CONTENT_PATTERN, PAGE_SIZE);
    if (val_host_rmi_rtt_data_map_init(c_args.state_realm.rd, data,
                                       IPA_ADDR_CONTENT, src,
                                       RMI_NO_MEASURE_CONTENT)) {
        LOG(ERROR, "Could not initialize the DATA Granule\n");
        return VAL_ERROR;
    }

    unmap_flags.oaddr_type = RMI_ADDR_TYPE_SINGLE;
    val_memcpy(&flags, &unmap_flags, sizeof(unmap_flags));
    if (val_host_rmi_rtt_data_unmap(c_args.state_realm.rd,
                                    IPA_ADDR_CONTENT,
                                    IPA_ADDR_CONTENT + PAGE_SIZE,
                                    flags, 0)) {
        LOG(ERROR, "Could not unmap the DATA Granule\n");
        return VAL_ERROR;
    }

    if (val_host_rmi_granule_range_undelegate(data, data + PAGE_SIZE)) {
        LOG(ERROR, "Could not undelegate the DATA Granule\n");
        return VAL_ERROR;
    }

    if (val_memcmp((void *)src, (void *)data, PAGE_SIZE) == 0) {
        LOG(ERROR, "Former DATA Granule still contains the source pattern\n");
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
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

void cmd_granule_range_undelegate_host(void)
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

        cmd_ret = val_host_rmi_granule_range_undelegate_smc(args.base, args.top);
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
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
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
    ret = val_host_rmi_granule_range_undelegate(c_args.base_valid,
                                                c_args.top_valid);
    if (ret != 0)
    {
        LOG(ERROR, "Granule range undelegate command failed, ret value is: %x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    LOG(TEST, "Check %2d : Content changed after undelegation\n", ++i);
    if (content_changed_check()) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    if (val_host_tracking_none_cleanup(&c_args.track_region) != VAL_SUCCESS)
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
    return;
}
