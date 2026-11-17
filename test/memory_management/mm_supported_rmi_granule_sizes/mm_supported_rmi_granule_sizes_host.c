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

static bool is_tgran4_supported(uint64_t mmfr0)
{
    uint64_t tgran4 = VAL_EXTRACT_BITS(mmfr0, ID_AA64MMFR0_EL1_TGRAN4_SHIFT,
                                       ID_AA64MMFR0_EL1_TGRAN4_SHIFT +
                                       ID_AA64MMFR0_EL1_TGRAN4_WIDTH - 1);

    return tgran4 != ID_AA64MMFR0_EL1_TGRAN4_NOT_SUPPORTED;
}

static bool is_tgran16_supported(uint64_t mmfr0)
{
    uint64_t tgran16 = VAL_EXTRACT_BITS(mmfr0, ID_AA64MMFR0_EL1_TGRAN16_SHIFT,
                                        ID_AA64MMFR0_EL1_TGRAN16_SHIFT +
                                        ID_AA64MMFR0_EL1_TGRAN16_WIDTH - 1);

    return tgran16 != ID_AA64MMFR0_EL1_TGRAN16_NOT_SUPPORTED;
}

static bool is_tgran64_supported(uint64_t mmfr0)
{
    uint64_t tgran64 = VAL_EXTRACT_BITS(mmfr0, ID_AA64MMFR0_EL1_TGRAN64_SHIFT,
                                        ID_AA64MMFR0_EL1_TGRAN64_SHIFT +
                                        ID_AA64MMFR0_EL1_TGRAN64_WIDTH - 1);

    return tgran64 != ID_AA64MMFR0_EL1_TGRAN64_NOT_SUPPORTED;
}

static bool is_rmi_granule_size_supported(uint64_t feature_reg, uint32_t bit)
{
    return VAL_EXTRACT_BITS(feature_reg, bit, bit) != 0;
}

void mm_supported_rmi_granule_sizes_host(void)
{
    uint64_t feature_reg;
    uint64_t mmfr0;
    uint64_t ret;

    ret = val_host_rmi_features(1, &feature_reg);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_FEATURES failed, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    mmfr0 = val_id_aa64mmfr0_el1_read();

    LOG(TEST, "RMI Feature Register 1: 0x%lx, ID_AA64MMFR0_EL1: 0x%lx\n",
        feature_reg, mmfr0);

    if (is_rmi_granule_size_supported(feature_reg,
                                      RMI_FEATURE_REGISTER_1_RMI_GRAN_SZ_4KB) &&
        !is_tgran4_supported(mmfr0))
    {
        LOG(ERROR, "RMI advertises 4KB granule, but 4KB translation granule is unsupported\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return;
    }

    if (is_rmi_granule_size_supported(feature_reg,
                                      RMI_FEATURE_REGISTER_1_RMI_GRAN_SZ_16KB) &&
        !is_tgran16_supported(mmfr0))
    {
        LOG(ERROR, "RMI advertises 16KB granule, but 16KB translation granule is unsupported\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        return;
    }

    if (is_rmi_granule_size_supported(feature_reg,
                                      RMI_FEATURE_REGISTER_1_RMI_GRAN_SZ_64KB) &&
        !is_tgran64_supported(mmfr0))
    {
        LOG(ERROR, "RMI advertises 64KB granule, but 64KB translation granule is unsupported\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        return;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));
}
