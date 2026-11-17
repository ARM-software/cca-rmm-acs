/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"

static uint64_t expected_rmi_granule_size(void)
{
#if (PHYSICAL_GRANULE_SIZE == PAGE_SIZE_4K)
    return RMI_GRANULE_SIZE_4KB;
#elif (PHYSICAL_GRANULE_SIZE == PAGE_SIZE_16K)
    return RMI_GRANULE_SIZE_16KB;
#elif (PHYSICAL_GRANULE_SIZE == PAGE_SIZE_64K)
    return RMI_GRANULE_SIZE_64KB;
#else
    return VAL_ERROR;
#endif
}

void mm_rmi_physical_granule_size_host(void)
{
    val_smc_param_ts cmd_ret;
    val_host_rmi_rmm_config_ts *cfg;
    uint64_t expected_size = expected_rmi_granule_size();

    cfg = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (cfg == NULL)
    {
        LOG(ERROR, "Failed to allocate RMM config structure\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    if (expected_size == VAL_ERROR)
    {
        LOG(ERROR, "Unsupported platform Physical Granule size: 0x%lx\n",
            PHYSICAL_GRANULE_SIZE);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto free_cfg;
    }

    val_memset(cfg, 0, PAGE_SIZE);
    cmd_ret = val_host_rmi_rmm_config_get((uint64_t)cfg);
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_RMM_CONFIG_GET failed, ret=0x%lx\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto free_cfg;
    }

    LOG(TEST, "Physical Granule size: 0x%lx, RMI Granule size encoding: 0x%x\n",
        PHYSICAL_GRANULE_SIZE, cfg->rmi_granule_size);

    if (cfg->rmi_granule_size != expected_size)
    {
        LOG(ERROR, "RMI Granule size 0x%x does not match Physical Granule size 0x%lx\n",
            cfg->rmi_granule_size, PHYSICAL_GRANULE_SIZE);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto free_cfg;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

free_cfg:
    val_host_mem_free(cfg);
}
