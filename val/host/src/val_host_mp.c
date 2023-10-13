/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_mp.h"

#define CONTEXT_ID_VALUE 0x5555

/**
 *   @brief    Power up the given core
 *   @param    target_cpuid     - Logical cpuid value of the core
 *   @return   SUCCESS/FAILURE
**/
uint64_t val_host_power_on_cpu(uint32_t target_cpuid)
{
    uint64_t target_cpu = val_get_mpidr(target_cpuid);
    uint64_t ret;

    ret = val_psci_cpu_on(target_cpu, val_host_get_secondary_cpu_entry(), CONTEXT_ID_VALUE);
    if (ret == PSCI_E_SUCCESS)
    {
        return VAL_SUCCESS;
    } else {
        LOG(WARN, "\tPSCI_CPU_ON failed, ret=0x%x\n", ret, 0);
        return VAL_ERROR;
    }
}

/**
 *   @brief    Power down the calling core.
 *   @param    void
 *   @return   The call does not return when successful. Otherwise, it returns VAL_ERROR.
**/
uint64_t val_host_power_off_cpu(void)
{
    uint64_t ret =  val_psci_cpu_off();

    LOG(WARN, "\tPSCI_CPU_OFF failed, ret=0x%x\n", ret, 0);
    return VAL_ERROR;
}
