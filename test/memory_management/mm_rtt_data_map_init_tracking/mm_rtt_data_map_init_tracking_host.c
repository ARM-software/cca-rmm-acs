/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_command.h"
#include "tracking_common_host.h"

#define DATA_MAP_INIT_IPA 0x800000
#define SIZE_2MB    (2UL * 1024UL * 1024UL)
#define SIZE_32MB   (32UL * 1024UL * 1024UL)
#define SIZE_512MB  (512UL * 1024UL * 1024UL)
#define SIZE_1GB    (1024UL * 1024UL * 1024UL)
#define SIZE_64GB   (64UL * SIZE_1GB)
#define SIZE_4TB    (4UL * 1024UL * SIZE_1GB)
#define RMI_RESULT_LEVEL_ADDR_LEVEL(result) VAL_EXTRACT_BITS((result), 8, 15)
#define RMI_RESULT_LEVEL_ADDR_ADDR(result)  VAL_EXTRACT_BITS((result), 16, 55)

static uint64_t rmi_granule_shift(uint64_t granule_size)
{
    switch (granule_size)
    {
        case RMI_GRANULE_SIZE_4KB:
            return 12;

        case RMI_GRANULE_SIZE_16KB:
            return 14;

        case RMI_GRANULE_SIZE_64KB:
            return 16;

        default:
            return 0;
    }
}

static uint64_t addr_shift_rmi_granule(uint64_t addr, uint64_t granule_size)
{
    uint64_t shift = rmi_granule_shift(granule_size);

    if (shift == 0)
        return 0;

    return (addr >> shift) & ((1UL << 40) - 1UL);
}

static uint64_t tracking_to_rmi_result(uint64_t tracking_state)
{
    return (tracking_state == RMI_TRACKING_COARSE) ? 0UL : 1UL;
}

static uint64_t tracking_region_size_from_rmi(uint64_t granule_size,
                                              uint64_t tracking_region_size)
{
    switch (granule_size)
    {
        case RMI_GRANULE_SIZE_4KB:
            if (tracking_region_size == 0)
                return SIZE_1GB;
            return (tracking_region_size == 1) ? SIZE_2MB : 0;

        case RMI_GRANULE_SIZE_16KB:
            if (tracking_region_size == 0)
                return SIZE_32MB;
            return (tracking_region_size == 1) ? SIZE_64GB : 0;

        case RMI_GRANULE_SIZE_64KB:
            if (tracking_region_size == 0)
                return SIZE_512MB;
            return (tracking_region_size == 1) ? SIZE_4TB : 0;

        default:
            return 0;
    }
}

static bool check_tracking_error_result(uint64_t result, uint64_t data,
                                        uint64_t rmi_granule_size)
{
    uint64_t expected_level = tracking_to_rmi_result(RMI_TRACKING_COARSE);
    uint64_t expected_addr = addr_shift_rmi_granule(data, rmi_granule_size);

    if (RMI_STATUS(result) != RMI_ERROR_TRACKING)
    {
        LOG(ERROR, "RMI_RTT_DATA_MAP_INIT returned 0x%lx, expected RMI_ERROR_TRACKING\n",
            result);
        return false;
    }

    if ((RMI_RESULT_LEVEL_ADDR_LEVEL(result) != expected_level) ||
        (RMI_RESULT_LEVEL_ADDR_ADDR(result) != expected_addr))
    {
        LOG(ERROR, "RMI_ERROR_TRACKING payload mismatch, ret=0x%lx, "
                   "expected level=0x%lx addr=0x%lx\n",
            result, expected_level, expected_addr);
        return false;
    }

    return true;
}

void mm_rtt_data_map_init_tracking_host(void)
{
    val_host_rmi_rmm_config_ts *cfg;
    val_host_tracking_test_region_ts tracking;
    val_smc_param_ts cmd_ret;
    val_host_realm_ts realm;
    uint64_t data;
    uint64_t src = 0;
    uint64_t tracking_region_size;
    uint64_t ret;

    cfg = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (cfg == NULL)
    {
        LOG(ERROR, "Failed to allocate RMM config structure\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    val_memset(cfg, 0, PAGE_SIZE);
    ret = val_host_rmi_rmm_config_get((uint64_t)cfg).x0;
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_RMM_CONFIG_GET failed, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto free_cfg;
    }

    tracking_region_size = tracking_region_size_from_rmi(cfg->rmi_granule_size,
                                                         cfg->tracking_region_size);
    if (tracking_region_size == 0U)
    {
        LOG(ERROR, "Unsupported RMI granule/tracking region configuration\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto free_cfg;
    }

    if (tracking_region_size != TRACKING_TEST_REGION_SIZE_2MB)
    {
        LOG(TEST, "Skipping: tracking test requires 2 MB regions\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto free_cfg;
    }

    ret = val_host_tracking_coarse_prep_sequence(&tracking);
    if (ret == VAL_SKIP_CHECK)
    {
        LOG(TEST, "Skipping: coarse tracking test region is unavailable\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto free_cfg;
    }
    if (ret != PLATFORM_TRACKING_TEST_REGION_BASE)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto cleanup;
    }
    data = ret;

    val_memset(&realm, 0, sizeof(realm));
    realm.s2sz = 40;
    realm.hash_algo = RMI_HASH_SHA_256;
    realm.s2_starting_level = 0;
    realm.num_s2_sl_rtts = 1;
    realm.vmid = 0;

    if (val_host_realm_create(&realm))
    {
        LOG(ERROR, "Realm create failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto cleanup;
    }

    if (create_mapping(DATA_MAP_INIT_IPA, true, realm.rd))
    {
        LOG(ERROR, "Couldn't create protected mapping\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto cleanup;
    }

    src = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (src == 0)
    {
        LOG(ERROR, "Failed to allocate DATA_MAP_INIT source granule\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto cleanup;
    }

    cmd_ret = val_host_rmi_rtt_data_map_init_smc(realm.rd, data, DATA_MAP_INIT_IPA,
                                                 src, RMI_MEASURE_CONTENT);
    ret = cmd_ret.x0;
    if (!check_tracking_error_result(ret, data, cfg->rmi_granule_size))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto cleanup;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    if (src != 0)
        val_host_mem_free((void *)src);

    if (val_host_tracking_coarse_cleanup(&tracking) != VAL_SUCCESS)
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));

free_cfg:
    val_host_mem_free(cfg);
}
