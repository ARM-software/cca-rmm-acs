/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"

#define RMI_FEATURE_REGISTER_1_RMI_GRAN_SZ_4KB    0
#define RMI_FEATURE_REGISTER_1_RMI_GRAN_SZ_16KB   1
#define RMI_FEATURE_REGISTER_1_RMI_GRAN_SZ_64KB   2

struct rmi_granule_size_info {
    uint32_t feature_bit;
    uint64_t size;
    const char *name;
};

static const struct rmi_granule_size_info rmi_granule_sizes[] = {
    {RMI_FEATURE_REGISTER_1_RMI_GRAN_SZ_4KB, PAGE_SIZE_4K, "4KB"},
    {RMI_FEATURE_REGISTER_1_RMI_GRAN_SZ_16KB, PAGE_SIZE_16K, "16KB"},
    {RMI_FEATURE_REGISTER_1_RMI_GRAN_SZ_64KB, PAGE_SIZE_64K, "64KB"},
};

static bool is_rmi_granule_size_supported(uint64_t feature_reg, uint32_t bit)
{
    return VAL_EXTRACT_BITS(feature_reg, bit, bit) != 0;
}

void mm_rmi_granule_size_not_smaller_host(void)
{
    uint64_t feature_reg;
    uint64_t ret;
    uint32_t i;

    ret = val_host_rmi_features(1, &feature_reg);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_FEATURES failed, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    LOG(TEST, "RMI Feature Register 1: 0x%lx, Physical Granule size: 0x%lx\n",
        feature_reg, PHYSICAL_GRANULE_SIZE);

    for (i = 0; i < (sizeof(rmi_granule_sizes) / sizeof(rmi_granule_sizes[0])); i++)
    {
        if (!is_rmi_granule_size_supported(feature_reg, rmi_granule_sizes[i].feature_bit))
        {
            continue;
        }

        if (rmi_granule_sizes[i].size < PHYSICAL_GRANULE_SIZE)
        {
            LOG(ERROR, "RMI advertises %s granule, smaller than Physical Granule 0x%lx\n",
                rmi_granule_sizes[i].name, PHYSICAL_GRANULE_SIZE);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(i + 2)));
            return;
        }
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));
}
