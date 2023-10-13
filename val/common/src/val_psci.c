/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_psci.h"

/**
 *   @brief    Returns the PSCI Affinity info for target affinity and level
 *   @param    target_affinity          - Target affinity
 *   @param    lower_affinity_level     - Lower affinity level
 *   @return   Returns the smc arg0
**/
uint64_t val_psci_affinity_info(uint64_t target_affinity,
                        uint64_t lower_affinity_level)
{
    return (val_smc_call(PSCI_AFFINITY_INFO_AARCH64,
                       target_affinity,
                       lower_affinity_level,
                       0, 0, 0, 0, 0, 0, 0, 0)).x0;
}

/**
 *   @brief    PSCI CPU on
 *   @param    target_cpu              - Target cpu
 *   @param    entry_point_address     - Entry point address
 *   @param    context_id              - conext id of target cpu
 *   @return   Returns the smc arg0
**/
uint64_t val_psci_cpu_on(uint64_t target_cpu,
                        uint64_t entry_point_address,
                        uint64_t context_id)
{
    return (val_smc_call(PSCI_CPU_ON_AARCH64,
                       target_cpu,
                       entry_point_address,
                       context_id,
                       0, 0, 0, 0, 0, 0, 0)).x0;
}

/**
 *   @brief    PSCI CPU off
 *   @param    void
 *   @return   Returns the smc arg0
**/
uint64_t val_psci_cpu_off(void)
{
    return (val_smc_call(PSCI_CPU_OFF,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0)).x0;
}

/**
 *   @brief    PSCI CPU suspend
 *   @param    power_state             - Power state of cpu
 *   @param    entry_point_address     - Entry point address
 *   @param    context_id              - conext id of target cpu
 *   @return   Returns the smc arg0
**/
uint64_t val_psci_cpu_suspend(uint64_t power_state,
                        uint64_t entry_point_address,
                        uint64_t context_id)
{
    return (val_smc_call(PSCI_CPU_SUSPEND_AARCH64,
                       power_state,
                       entry_point_address,
                       context_id,
                       0, 0, 0, 0, 0, 0, 0)).x0;
}

/**
 *   @brief    PSCI features
 *   @param    psci_func_id     - PSCI function id
 *   @return   Returns the smc arg0
**/
uint64_t val_psci_features(uint64_t psci_func_id)
{
    return (val_smc_call(PSCI_FEATURES,
                       psci_func_id,
                       0, 0, 0, 0, 0, 0, 0, 0, 0)).x0;
}

/**
 *   @brief    PSCI system off
 *   @param    void
 *   @return   Returns the smc arg0
**/
uint64_t val_psci_system_off(void)
{
    return (val_smc_call(PSCI_SYSTEM_OFF,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0)).x0;
}

/**
 *   @brief    PSCI system reset
 *   @param    void
 *   @return   Returns the smc arg0
**/
uint64_t val_psci_system_reset(void)
{
    return (val_smc_call(PSCI_SYSTEM_RESET,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0)).x0;
}

/**
 *   @brief    PSCI version
 *   @param    void
 *   @return   Returns the smc arg0
**/
uint64_t val_psci_version(void)
{
    return (val_smc_call(PSCI_VERSION,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0)).x0;
}
