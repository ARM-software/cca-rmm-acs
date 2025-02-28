/*
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_rmi.h"
#include "val_host_lfa.h"
#include "val_libc.h"
#include "val_host_realm.h"

/**
 *   @brief    Determine the version of the LFA ABI
 *   @param    none
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_lfa_version(void)
{
    return val_smc_call(LFA_VERSION, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Determine the existence of functions in the LFA ABI
 *   @param    lfa_fid      - fid of the LFA ABI function to query features from.
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_lfa_features(uint64_t lfa_fid)
{
    return val_smc_call(LFA_FEATURES, lfa_fid, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Obtain information about the LFA behaviour and current configuration
 *   @param    lfa_info_selector      - Selects the information returned.
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_lfa_get_info(uint64_t lfa_info_selector)
{
    return val_smc_call(LFA_GET_INFO, lfa_info_selector, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Discovers the firmware components that are managed by the LFA
 *   @param    fw_seq_id      - The sequence ID of the firmware component
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_lfa_get_inventory(uint64_t fw_seq_id)
{
    return val_smc_call(LFA_GET_INVENTORY, fw_seq_id, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Prepare the platform for a live firmware activation
 *   @param    fw_seq_id      - The sequence ID of the firmware component
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_lfa_prime(uint64_t fw_seq_id)
{
    return val_smc_call(LFA_PRIME, fw_seq_id, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

