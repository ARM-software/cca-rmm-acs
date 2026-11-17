/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "val_host_tracking.h"
#include "val_host_alloc.h"
#include "val_host_realm.h"
#include "val_host_rmi.h"
#include "val_host_sro.h"

#define VAL_TRACKING_REGION_SIZE_2MB (1UL << 21)
#define VAL_TRACKING_REGION_SIZE_1GB (1UL << 30)

/* CONFIG_GET/SET need an RMI-granule-aligned Non-secure buffer. */
static uint8_t rmm_config_buffer[PAGE_SIZE_64K]
    __attribute__((aligned(PAGE_SIZE_64K)));

typedef struct {
    uint64_t base;
    uint64_t top;
    uint64_t expected_category;
} val_tracking_device_range_t;

static const val_tracking_device_range_t device_tracking_ranges[] = {
    PLATFORM_RMM_DEVICE_TRACKING_RANGES
};

static const char *val_host_tracking_device_category_name(uint64_t category)
{
    if (category == RMI_MEM_CATEGORY_DEV_NCOH)
        return "NCOH";
    if (category == RMI_MEM_CATEGORY_DEV_COH)
        return "COH";

    return "UNKNOWN";
}

static uint64_t val_host_tracking_region_size(void)
{
    if (PLATFORM_RMM_RMI_GRANULE_SIZE != RMI_GRANULE_SIZE_4KB)
        return 0U;

    if (PLATFORM_RMM_TRACKING_REGION_SIZE == 0U)
        return VAL_TRACKING_REGION_SIZE_1GB;
    if (PLATFORM_RMM_TRACKING_REGION_SIZE == 1U)
        return VAL_TRACKING_REGION_SIZE_2MB;

    return 0U;
}

static uint32_t val_host_tracking_read_region(uint64_t addr,
                                              uint64_t *category,
                                              uint64_t *state)
{
    val_smc_param_ts ret;

    if (addr > UINT64_MAX - PAGE_SIZE)
        return VAL_ERROR;

    ret = val_host_rmi_granule_tracking_get(addr, addr + PAGE_SIZE);

    if (ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "Tracking GET failed at 0x%lx, ret=0x%lx\n",
            addr, ret.x0);
        return VAL_ERROR;
    }

    if ((ret.x1 > RMI_MEM_CATEGORY_NONE) ||
        (ret.x2 > RMI_TRACKING_INTERMEDIATE))
    {
        LOG(ERROR, "Invalid tracking category/state at 0x%lx: "
            "category=0x%lx state=0x%lx\n", addr, ret.x1, ret.x2);
        return VAL_ERROR;
    }

    if (ret.x3 < addr + PAGE_SIZE)
    {
        LOG(ERROR, "Tracking GET range ends at 0x%lx before 0x%lx\n",
            ret.x3, addr + PAGE_SIZE);
        return VAL_ERROR;
    }

    *category = ret.x1;
    *state = ret.x2;

    return VAL_SUCCESS;
}

static uint32_t val_host_tracking_set_fine(uint64_t region_base,
                                           uint64_t region_size,
                                           uint64_t probe_addr,
                                           uint64_t category)
{
    val_smc_param_ts ret;
    uint64_t actual_category;
    uint64_t actual_state;

    ret = val_host_rmi_granule_tracking_set_smc(
        region_base, category, RMI_TRACKING_FINE);
    if (val_host_sro_complete_tracking(&ret, region_base,
                                       region_size) != RMI_SUCCESS)
    {
        LOG(ERROR, "TRACKING_SET to FINE failed at 0x%lx, ret=0x%lx\n",
            region_base, ret.x0);
        return VAL_ERROR;
    }

    if (val_host_tracking_read_region(probe_addr, &actual_category,
                                      &actual_state) ||
        (actual_category != category) ||
        (actual_state != RMI_TRACKING_FINE))
    {
        LOG(ERROR, "Tracking region 0x%lx is not FINE after SET\n",
            region_base);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint32_t val_host_tracking_configure_device_region(
                uint64_t region_base, uint64_t probe_addr, uint64_t category,
                uint64_t region_size, bool rmm_active_on_entry)
{
    uint64_t current_category;
    uint64_t current_state;

    if (val_host_tracking_read_region(probe_addr,
                                      &current_category, &current_state))
        return VAL_ERROR;

    if ((current_category != category) ||
        (current_state == RMI_TRACKING_RESERVED))
    {
        LOG(ERROR, "Device address 0x%lx has incompatible tracking: "
            "expected_category=0x%lx category=0x%lx state=0x%lx\n",
            probe_addr, category, current_category, current_state);
        return VAL_ERROR;
    }

    if (current_state != RMI_TRACKING_FINE)
    {
        if (rmm_active_on_entry)
        {
            LOG(ERROR, "Active RMM device region 0x%lx is not FINE, state=0x%lx\n",
                region_base, current_state);
            return VAL_ERROR;
        }

        if (val_host_tracking_set_fine(region_base, region_size, probe_addr,
                                       category))
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint32_t val_host_tracking_configure_device_regions(
    uint64_t region_size, bool rmm_active_on_entry)
{
    size_t i;
    uint64_t region_base;
    uint64_t probe_addr;

    for (i = 0; i < sizeof(device_tracking_ranges) /
                    sizeof(device_tracking_ranges[0]); i++)
    {
        if (device_tracking_ranges[i].base >= device_tracking_ranges[i].top)
        {
            LOG(ERROR, "Invalid device tracking range at index %lu\n", i);
            return VAL_ERROR;
        }

        region_base = ADDR_ALIGN_DOWN(device_tracking_ranges[i].base,
                                      region_size);
        for (; region_base < device_tracking_ranges[i].top;
             region_base += region_size)
        {
            probe_addr = region_base;
            if (probe_addr < device_tracking_ranges[i].base)
                probe_addr = device_tracking_ranges[i].base;

            if (val_host_tracking_configure_device_region(
                    region_base, probe_addr,
                    device_tracking_ranges[i].expected_category,
                    region_size, rmm_active_on_entry))
                return VAL_ERROR;
        }

        LOG(TEST, "Device %s: FINE [0x%lx, 0x%lx)\n",
            val_host_tracking_device_category_name(
                device_tracking_ranges[i].expected_category),
            device_tracking_ranges[i].base, device_tracking_ranges[i].top);
    }

    return VAL_SUCCESS;
}

static uint32_t val_host_tracking_init_dram_region(uint64_t region_base,
                                                   uint64_t region_size,
                                                   bool rmm_active_on_entry)
{
    uint64_t category;
    uint64_t state;

    if (val_host_tracking_read_region(region_base, &category, &state))
        return VAL_ERROR;

    if ((category != RMI_MEM_CATEGORY_CONVENTIONAL) ||
        (state == RMI_TRACKING_RESERVED))
    {
        LOG(ERROR, "DRAM tracking region 0x%lx is incompatible\n",
            region_base);
        return VAL_ERROR;
    }

    if (state == RMI_TRACKING_FINE)
        return VAL_SUCCESS;

    /* An already-active RMM may own setup-pool granules from an earlier run.
     * A new SET could request memory that ACS cannot safely allocate.
     */
    if (rmm_active_on_entry)
    {
        LOG(ERROR, "Active RMM DRAM region 0x%lx is not FINE\n", region_base);
        return VAL_ERROR;
    }

    if (val_host_tracking_set_fine(region_base, region_size, region_base,
                                   RMI_MEM_CATEGORY_CONVENTIONAL))
        return VAL_ERROR;

    return VAL_SUCCESS;
}

uint32_t val_host_tracking_init(void)
{
    val_host_rmi_rmm_config_ts *cfg =
        (val_host_rmi_rmm_config_ts *)rmm_config_buffer;
    val_smc_param_ts ret;
    uint64_t region_size;
    uint64_t region_base;
    uint64_t first_pool_region;
    uint64_t pool_end;
    uint64_t feature_reg;
    uint64_t feature_status;
    bool rmm_active_on_entry = false;
    bool config_updated = false;

    region_size = val_host_tracking_region_size();
    if ((region_size == 0U) ||
        (PLATFORM_RMM_TRACKING_REGION_BASE !=
         ADDR_ALIGN_DOWN(PLATFORM_NORMAL_WORLD_IMAGE_BASE, region_size)))
    {
        LOG(ERROR, "Unsupported or unaligned platform tracking region\n");
        return VAL_ERROR;
    }

    ret = val_host_rmi_rmm_state_get();
    if ((ret.x0 != RMI_SUCCESS) || (ret.x1 > RMI_RMM_STATE_ACTIVE))
    {
        LOG(ERROR, "RMM_STATE_GET failed, ret=0x%lx state=0x%lx\n",
            ret.x0, ret.x1);
        return VAL_ERROR;
    }
    if (ret.x1 == RMI_RMM_STATE_ACTIVE)
        rmm_active_on_entry = true;

    LOG(TEST, "ACS tracking setup:\n");
    LOG(TEST, "RMM state on entry: %s\n",
        rmm_active_on_entry ? "ACTIVE" : "INIT");

    if (!rmm_active_on_entry)
        val_host_tracking_alloc_init();

    feature_status = val_host_rmi_features(1, &feature_reg);
    if (feature_status != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_FEATURES(1) failed, ret=0x%lx\n", feature_status);
        return VAL_ERROR;
    }

    LOG(TEST, "RMI granule support: 4 KB=%lu, 16 KB=%lu, 64 KB=%lu\n",
        VAL_EXTRACT_BITS(feature_reg, 0, 0),
        VAL_EXTRACT_BITS(feature_reg, 1, 1),
        VAL_EXTRACT_BITS(feature_reg, 2, 2));
    if (VAL_EXTRACT_BITS(feature_reg, 0, 0) != RMI_FEATURE_TRUE)
    {
        LOG(ERROR, "ACS tracking setup requires 4KB RMI granules\n");
        return VAL_ERROR;
    }

    val_memset(cfg, 0, sizeof(rmm_config_buffer));
    ret = val_host_rmi_rmm_config_get((uint64_t)cfg);
    if (ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "RMM_CONFIG_GET failed, ret=0x%lx\n", ret.x0);
        return VAL_ERROR;
    }

    if ((cfg->rmi_granule_size != PLATFORM_RMM_RMI_GRANULE_SIZE) ||
        (cfg->tracking_region_size != PLATFORM_RMM_TRACKING_REGION_SIZE))
    {
        if (rmm_active_on_entry)
        {
            LOG(ERROR, "Active RMM has incompatible tracking configuration\n");
            return VAL_ERROR;
        }

        cfg->rmi_granule_size = PLATFORM_RMM_RMI_GRANULE_SIZE;
        cfg->tracking_region_size = PLATFORM_RMM_TRACKING_REGION_SIZE;
        ret = val_host_rmi_rmm_config_set((uint64_t)cfg);
        if (ret.x0 != RMI_SUCCESS)
        {
            LOG(ERROR, "RMM_CONFIG_SET failed, ret=0x%lx\n", ret.x0);
            return VAL_ERROR;
        }

        val_memset(cfg, 0, sizeof(rmm_config_buffer));
        ret = val_host_rmi_rmm_config_get((uint64_t)cfg);
        if ((ret.x0 != RMI_SUCCESS) ||
            (cfg->rmi_granule_size != PLATFORM_RMM_RMI_GRANULE_SIZE) ||
            (cfg->tracking_region_size != PLATFORM_RMM_TRACKING_REGION_SIZE))
        {
            LOG(ERROR, "RMM did not retain the platform tracking configuration\n");
            return VAL_ERROR;
        }

        config_updated = true;
    }

    if (config_updated)
    {
        LOG(TEST, "ACS configured RMM: 4 KB granules, %lu MB tracking regions\n",
            region_size >> 20);
    }
    else
    {
        LOG(TEST, "RMM already uses: 4 KB granules, "
            "%lu MB tracking regions\n", region_size >> 20);
    }

    if (!rmm_active_on_entry && val_host_rmm_activate())
        return VAL_ERROR;

    first_pool_region = ADDR_ALIGN_DOWN(PLATFORM_MEMORY_POOL_BASE, region_size);
    pool_end = PLATFORM_MEMORY_POOL_BASE + PLATFORM_MEMORY_POOL_SIZE;

    /*
     * Fine-track the memory pool first. It contains the setup memory that
     * TRACKING_SET may request while we process the ACS image.
     */
    for (region_base = first_pool_region;
         region_base < pool_end; region_base += region_size)
    {
        if (val_host_tracking_init_dram_region(region_base, region_size,
                                               rmm_active_on_entry))
            return VAL_ERROR;
    }

    /*
     * Now fine-track the image regions before the pool.
     * In 1 GB mode the image and pool share one region, so this loop is empty.
     */
    for (region_base = PLATFORM_RMM_TRACKING_REGION_BASE;
         region_base < first_pool_region; region_base += region_size)
    {
        if (val_host_tracking_init_dram_region(region_base, region_size,
                                               rmm_active_on_entry))
            return VAL_ERROR;
    }

    LOG(TEST, "ACS DRAM: FINE [0x%lx, 0x%lx)\n",
        PLATFORM_RMM_TRACKING_REGION_BASE, ADDR_ALIGN(pool_end, region_size));

    if (val_host_rmm_supports_da() &&
        val_host_tracking_configure_device_regions(region_size,
                                                    rmm_active_on_entry))
        return VAL_ERROR;

    return VAL_SUCCESS;
}
