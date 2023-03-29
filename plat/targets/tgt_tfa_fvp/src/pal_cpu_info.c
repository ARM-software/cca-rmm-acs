/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_interfaces.h"

/* Populate phy_mpid_array with mpidr value of CPUs available
 * in the system. */
static const uint64_t phy_mpidr_array[PLATFORM_CPU_COUNT] = {
    PLATFORM_PHY_MPIDR_CPU0,
    PLATFORM_PHY_MPIDR_CPU1,
#if (PLATFORM_CPU_COUNT > 2)
    PLATFORM_PHY_MPIDR_CPU2,
#endif
#if (PLATFORM_CPU_COUNT > 3)
    PLATFORM_PHY_MPIDR_CPU3,
#endif
#if (PLATFORM_CPU_COUNT > 4)
    PLATFORM_PHY_MPIDR_CPU4,
#endif
#if (PLATFORM_CPU_COUNT > 5)
    PLATFORM_PHY_MPIDR_CPU5,
#endif
#if (PLATFORM_CPU_COUNT > 6)
    PLATFORM_PHY_MPIDR_CPU6,
#endif
#if (PLATFORM_CPU_COUNT > 7)
    PLATFORM_PHY_MPIDR_CPU7,
#endif
};

uint32_t pal_get_cpu_count(void)
{
    return PLATFORM_CPU_COUNT;
}

uint64_t *pal_get_phy_mpidr_list_base(void)
{
    return (uint64_t *)&phy_mpidr_array[0];
}
