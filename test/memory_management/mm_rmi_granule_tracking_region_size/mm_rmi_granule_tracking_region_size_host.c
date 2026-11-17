/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"

#define SIZE_2MB    (2UL * 1024UL * 1024UL)
#define SIZE_32MB   (32UL * 1024UL * 1024UL)
#define SIZE_512MB  (512UL * 1024UL * 1024UL)
#define SIZE_1GB    (1024UL * 1024UL * 1024UL)
#define SIZE_64GB   (64UL * SIZE_1GB)
#define SIZE_4TB    (4UL * 1024UL * SIZE_1GB)

static uint64_t tracking_region_size_from_rmi(uint64_t granule_size,
                                              uint64_t tracking_region_size)
{
    switch (granule_size)
    {
        case RMI_GRANULE_SIZE_4KB:
            if (tracking_region_size == 0)
            {
                return SIZE_1GB;
            }
            if (tracking_region_size == 1)
            {
                return SIZE_2MB;
            }
            break;

        case RMI_GRANULE_SIZE_16KB:
            if (tracking_region_size == 0)
            {
                return SIZE_32MB;
            }
            if (tracking_region_size == 1)
            {
                return SIZE_64GB;
            }
            break;

        case RMI_GRANULE_SIZE_64KB:
            if (tracking_region_size == 0)
            {
                return SIZE_512MB;
            }
            if (tracking_region_size == 1)
            {
                return SIZE_4TB;
            }
            break;

        default:
            break;
    }

    return 0;
}

void mm_rmi_granule_tracking_region_size_host(void)
{
    val_smc_param_ts cmd_ret;
    val_host_rmi_rmm_config_ts *cfg;
    uint64_t decoded_tracking_region_size;

    cfg = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (cfg == NULL)
    {
        LOG(ERROR, "Failed to allocate RMM config structure\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    val_memset(cfg, 0, PAGE_SIZE);
    cmd_ret = val_host_rmi_rmm_config_get((uint64_t)cfg);
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_RMM_CONFIG_GET failed, ret=0x%lx\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto free_cfg;
    }

    decoded_tracking_region_size = tracking_region_size_from_rmi(
        cfg->rmi_granule_size, cfg->tracking_region_size);

    LOG(TEST, "RMI Granule size encoding: 0x%x, Tracking region size encoding: 0x%x\n",
        cfg->rmi_granule_size, cfg->tracking_region_size);
    LOG(TEST, "Decoded tracking region size: 0x%lx\n", decoded_tracking_region_size);

    if (decoded_tracking_region_size == 0)
    {
        LOG(ERROR, "Invalid RMI granule/tracking region size combination: 0x%x/0x%x\n",
            cfg->rmi_granule_size, cfg->tracking_region_size);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto free_cfg;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

free_cfg:
    val_host_mem_free(cfg);
}
