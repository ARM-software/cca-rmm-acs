/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "tracking_common_host.h"
#include "val_host_sro.h"
#include "pal_common_support.h"

uint32_t val_host_tracking_test_range_transition(uint64_t base, uint64_t top,
                                                 bool delegate,
                                                 uint64_t *completed_top)
{
    val_smc_param_ts cmd_ret;
    uint64_t next = base;
    uint64_t ret;
    uint32_t busy_count = 0U;

    *completed_top = base;
    while (next < top)
    {
        if (delegate)
            cmd_ret = val_host_rmi_granule_range_delegate_smc(next, top);
        else
            cmd_ret = val_host_rmi_granule_range_undelegate_smc(next, top);

        if (RMI_STATUS(cmd_ret.x0) == RMI_BUSY)
        {
            if (++busy_count >= VAL_HOST_SRO_MAX_ITERATIONS)
            {
                LOG(ERROR, "Tracking range command busy at 0x%lx\n", next);
                return VAL_ERROR;
            }
            continue;
        }

        busy_count = 0U;
        ret = val_host_sro_complete(&cmd_ret);
        if ((RMI_STATUS(ret) != RMI_SUCCESS) ||
            (cmd_ret.x1 <= next) || (cmd_ret.x1 > top))
        {
            LOG(ERROR, "Tracking range command failed at 0x%lx, ret=0x%lx\n",
                next, ret);
            return VAL_ERROR;
        }

        next = cmd_ret.x1;
        *completed_top = next;
    }

    return VAL_SUCCESS;
}

uint64_t val_host_tracking_coarse_prep_sequence(val_host_tracking_test_region_ts *region)
{
    const uint64_t base = PLATFORM_TRACKING_TEST_REGION_BASE;
    const uint64_t top = base + TRACKING_TEST_REGION_SIZE_2MB;
    val_smc_param_ts ret;

    region->base = base;
    region->delegated_start = base;
    region->delegated_top = base;
    region->initial_state = RMI_TRACKING_COARSE;
    region->state_changed = false;

    if (PLATFORM_RMM_TRACKING_REGION_SIZE != 1U)
    {
        LOG(TEST, "Tracking check requires 2 MB regions\n");
        return VAL_SKIP_CHECK;
    }

    ret = val_host_rmi_granule_tracking_get(base, top);
    if (ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "Tracking test region TRACKING_GET failed, ret=0x%lx\n",
            ret.x0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if ((ret.x1 != RMI_MEM_CATEGORY_CONVENTIONAL) || (ret.x3 != top))
    {
        LOG(ERROR, "Tracking test region is not conventional 2 MB DRAM: "
            "category=0x%lx, top=0x%lx\n", ret.x1, ret.x3);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (ret.x2 == RMI_TRACKING_INTERMEDIATE)
    {
        LOG(TEST, "Tracking test region is INTERMEDIATE\n");
        return VAL_SKIP_CHECK;
    }

    region->initial_state = ret.x2;
    if ((ret.x2 == RMI_TRACKING_NONE) ||
        (ret.x2 == RMI_TRACKING_FINE))
    {
        if (val_host_rmi_granule_tracking_set(base,
                RMI_MEM_CATEGORY_CONVENTIONAL, RMI_TRACKING_COARSE) != RMI_SUCCESS)
        {
            LOG(ERROR, "Could not set tracking test region to COARSE\n");
            return VAL_TEST_PREP_SEQ_FAILED;
        }
        region->state_changed = true;
    }
    else if (ret.x2 != RMI_TRACKING_COARSE)
    {
        LOG(ERROR, "Tracking test region has invalid state 0x%lx\n", ret.x2);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    ret = val_host_rmi_granule_tracking_get(base, top);
    if ((ret.x0 != RMI_SUCCESS) ||
        (ret.x1 != RMI_MEM_CATEGORY_CONVENTIONAL) ||
        (ret.x2 != RMI_TRACKING_COARSE) || (ret.x3 != top))
    {
        LOG(ERROR, "Tracking test region is not COARSE after setup\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (val_host_tracking_test_range_transition(base, top, true,
                                                &region->delegated_top) != VAL_SUCCESS)
        return VAL_TEST_PREP_SEQ_FAILED;

    return base;
}

uint32_t val_host_tracking_coarse_cleanup(val_host_tracking_test_region_ts *region)
{
    val_smc_param_ts ret;
    uint64_t completed_top;

    if (region->delegated_top > region->delegated_start)
    {
        if (val_host_tracking_test_range_transition(region->delegated_start,
                                                    region->delegated_top, false,
                                                    &completed_top) != VAL_SUCCESS)
        {
            region->delegated_start = completed_top;
            LOG(ERROR, "Tracking test region undelegation failed at 0x%lx\n",
                completed_top);
            return VAL_ERROR;
        }
        region->delegated_start = region->base;
        region->delegated_top = region->base;
    }

    if (region->state_changed)
    {
        if (region->initial_state == RMI_TRACKING_FINE)
        {
            /* Restoring FINE may need memory from the tracking setup pool. */
            ret = val_host_rmi_granule_tracking_set_smc(region->base,
                    RMI_MEM_CATEGORY_CONVENTIONAL, RMI_TRACKING_FINE);
            if (val_host_sro_complete_tracking(&ret, region->base,
                    TRACKING_TEST_REGION_SIZE_2MB) != RMI_SUCCESS)
            {
                LOG(ERROR, "Could not restore tracking test region to FINE, ret=0x%lx\n",
                    ret.x0);
                return VAL_ERROR;
            }
        }
        else if (val_host_rmi_granule_tracking_set(region->base,
                RMI_MEM_CATEGORY_CONVENTIONAL, region->initial_state) != RMI_SUCCESS)
        {
            LOG(ERROR, "Could not restore tracking test region to state 0x%lx\n",
                region->initial_state);
            return VAL_ERROR;
        }
        ret = val_host_rmi_granule_tracking_get(
                  region->base, region->base + TRACKING_TEST_REGION_SIZE_2MB);
        if ((ret.x0 != RMI_SUCCESS) ||
            (ret.x1 != RMI_MEM_CATEGORY_CONVENTIONAL) ||
            (ret.x2 != region->initial_state) ||
            (ret.x3 != region->base + TRACKING_TEST_REGION_SIZE_2MB))
        {
            LOG(ERROR, "Tracking test region state was not restored\n");
            return VAL_ERROR;
        }
        region->state_changed = false;
    }

    return VAL_SUCCESS;
}
