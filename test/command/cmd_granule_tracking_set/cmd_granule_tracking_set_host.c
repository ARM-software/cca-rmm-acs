/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_rmi_sro.h"
#include "rmi_granule_tracking_set_data.h"
#include "command_common_host.h"

static struct argument_store {
    uint64_t addr_valid;
    uint64_t granule_size;
    uint64_t region_size;
    uint64_t state_valid;
    uint64_t category_valid;
    uint64_t out_top;
    void *allocation;
} c_args;

struct arguments {
    uint64_t addr;
    uint64_t category;
    uint64_t state;
};

static uint64_t config_to_sizes(val_host_rmi_rmm_config_ts *cfg)
{
    switch (cfg->rmi_granule_size)
    {
        case RMI_GRANULE_SIZE_4KB:
            c_args.granule_size = PAGE_SIZE_4K;
            if (cfg->tracking_region_size == 0)
                c_args.region_size = 1ULL << 30;
            else if (cfg->tracking_region_size == 1)
                c_args.region_size = 1ULL << 21;
            else
                return VAL_ERROR;
            break;

        case RMI_GRANULE_SIZE_16KB:
            c_args.granule_size = PAGE_SIZE_16K;
            if (cfg->tracking_region_size == 0)
                c_args.region_size = 1ULL << 25;
            else if (cfg->tracking_region_size == 1)
                c_args.region_size = 1ULL << 36;
            else
                return VAL_ERROR;
            break;

        case RMI_GRANULE_SIZE_64KB:
            c_args.granule_size = PAGE_SIZE_64K;
            if (cfg->tracking_region_size == 0)
                c_args.region_size = 1ULL << 29;
            else if (cfg->tracking_region_size == 1)
                c_args.region_size = 1ULL << 42;
            else
                return VAL_ERROR;
            break;

        default:
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    val_host_rmi_rmm_config_ts *cfg;
    val_smc_param_ts cmd_ret;
    uint64_t ret;

    val_memset(&c_args, 0, sizeof(c_args));
    cfg = val_host_mem_alloc(PAGE_SIZE_64K, PAGE_SIZE_64K);
    if (cfg == NULL)
        return VAL_TEST_PREP_SEQ_FAILED;

    val_memset(cfg, 0, PAGE_SIZE_64K);
    cmd_ret = val_host_rmi_rmm_config_get((uint64_t)cfg);
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        val_host_mem_free(cfg);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    ret = config_to_sizes(cfg);
    val_host_mem_free(cfg);
    if (ret != VAL_SUCCESS)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.allocation = val_host_mem_alloc(c_args.granule_size,
                                            c_args.granule_size);
    if (c_args.allocation == NULL)
        return VAL_TEST_PREP_SEQ_FAILED;
    c_args.addr_valid = ADDR_ALIGN_DOWN((uint64_t)c_args.allocation,
                                        c_args.region_size);

    cmd_ret = val_host_rmi_granule_tracking_get(c_args.addr_valid,
                   c_args.addr_valid + c_args.granule_size);
    if ((cmd_ret.x0 != RMI_SUCCESS) ||
        (VAL_EXTRACT_BITS(cmd_ret.x1, 2, 63) != 0) ||
        (VAL_EXTRACT_BITS(cmd_ret.x2, 3, 63) != 0))
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.category_valid = cmd_ret.x1;
    c_args.state_valid = cmd_ret.x2;
    c_args.out_top = cmd_ret.x3;
    /* Coarse donation must be outside the target region; the allocator cannot ensure it. */
    if ((c_args.category_valid != RMI_MEM_CATEGORY_CONVENTIONAL) ||
        (c_args.state_valid == RMI_TRACKING_RESERVED) ||
        (c_args.state_valid == RMI_TRACKING_COARSE))
        return VAL_SKIP_CHECK;
    if ((c_args.state_valid > RMI_TRACKING_COARSE) ||
        (c_args.out_top <= c_args.addr_valid))
        return VAL_TEST_PREP_SEQ_FAILED;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    args->addr = c_args.addr_valid;
    args->category = c_args.category_valid;
    args->state = c_args.state_valid;

    switch (label)
    {
        case STATE_INVALID:
            args->state = RMI_TRACKING_RESERVED;
            break;

        case ADDR_UNALIGNED:
            args->addr = g_unaligned_prep_sequence(c_args.addr_valid);
            break;

        case ADDR_BOUND:
            args->addr = ADDR_ALIGN_DOWN(1ULL << val_get_pa_range_supported(),
                                         c_args.region_size) + c_args.region_size;
            break;

        case CATEGORY_INCOMPATIBLE:
            args->category = RMI_MEM_CATEGORY_DEV_NCOH;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t tracking_config_matches_valid(void)
{
    val_smc_param_ts cmd_ret = val_host_rmi_granule_tracking_get(
        c_args.addr_valid, c_args.addr_valid + c_args.granule_size);

    if ((cmd_ret.x0 != RMI_SUCCESS) ||
        (cmd_ret.x1 != c_args.category_valid) ||
        (cmd_ret.x2 != c_args.state_valid) ||
        (cmd_ret.x3 != c_args.out_top))
        return VAL_ERROR;
    return VAL_SUCCESS;
}

static uint64_t reserved_region_sequence(void)
{
#ifdef PLATFORM_TRACKING_RESERVED_REGION_BASE
    uint64_t addr = PLATFORM_TRACKING_RESERVED_REGION_BASE;
    val_smc_param_ts cmd_ret;

    if (((addr & (c_args.region_size - 1U)) != 0U) ||
        (addr > (~0ULL - c_args.granule_size)))
    {
        LOG(ERROR, "Invalid Reserved tracking region address\n");
        return VAL_ERROR;
    }

    cmd_ret = val_host_rmi_granule_tracking_get(addr, addr + c_args.granule_size);
    if ((cmd_ret.x0 != RMI_SUCCESS) ||
        (cmd_ret.x1 > RMI_MEM_CATEGORY_DEV_COH) ||
        (cmd_ret.x2 != RMI_TRACKING_RESERVED))
    {
        LOG(ERROR, "Configured tracking region is not Reserved, ret=0x%x\n",
            cmd_ret.x0);
        return VAL_ERROR;
    }

    cmd_ret = val_host_rmi_granule_tracking_set_smc(
        addr, cmd_ret.x1, RMI_TRACKING_NONE);
    if (cmd_ret.x0 != PACK_CODE(RMI_ERROR_INPUT, 0))
    {
        LOG(ERROR, "Reserved tracking region SET returned 0x%x\n", cmd_ret.x0);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
#else
    return VAL_SKIP_CHECK;
#endif
}

void cmd_granule_tracking_set_host(void)
{
    uint64_t i, ret, expected_ret;
    val_smc_param_ts cmd_ret;
    struct arguments args;

    ret = valid_input_args_prep_sequence();
    if (ret == VAL_SKIP_CHECK)
    {
        LOG(TEST, "No suitable conventional tracking region available\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto free_addr;
    }
    if (ret != VAL_SUCCESS)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto free_addr;
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

        cmd_ret = val_host_rmi_granule_tracking_set_smc(
            args.addr, args.category, args.state);
        expected_ret = PACK_CODE(test_data[i].status, test_data[i].index);
        if (cmd_ret.x0 != expected_ret)
        {
            LOG(ERROR, "RMI_GRANULE_TRACKING_SET returned 0x%x, expected 0x%x\n",
                cmd_ret.x0, expected_ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto free_addr;
        }
    }

    LOG(TEST, "Check %2d : reserved\n", ++i);
    ret = reserved_region_sequence();
    if (ret == VAL_SKIP_CHECK)
    {
        LOG(TEST, "Reserved tracking region not configured; check skipped\n");
    }
    else if (ret != VAL_SUCCESS)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto free_addr;
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);
    ret = val_host_rmi_granule_tracking_set(c_args.addr_valid,
            c_args.category_valid, c_args.state_valid);
    if ((ret != RMI_SUCCESS) || (tracking_config_matches_valid() != VAL_SUCCESS))
    {
        LOG(ERROR, "Tracking configuration set/readback failed, ret=0x%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto free_addr;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

free_addr:
    if (c_args.allocation != NULL)
        val_host_mem_free(c_args.allocation);
}
