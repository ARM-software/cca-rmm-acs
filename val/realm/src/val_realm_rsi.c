/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "val_realm_rsi.h"
#include "val_realm_framework.h"

__attribute__((aligned (PAGE_SIZE))) static uint8_t realm_config_buff[PAGE_SIZE];

__attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t gv_realm_host_call;

/**
 *   @brief    Returns RSI version
 *   @param    void
 *   @return   Returns RSI Interface version
**/
uint64_t val_realm_rsi_version(void)
{
    return (val_smc_call(RSI_VERSION, 0, 0, 0, 0, 0, 0, 0)).x0;
}

/**
 *   @brief    Read configuration for the current Realm
 *   @param    addr     - IPA of the Granule to which the configuration data will be written
 *   @return   Returns command return status
**/
uint64_t val_realm_rsi_realm_config(uint64_t buff)
{
    return (val_smc_call(RSI_REALM_CONFIG, buff, 0, 0, 0, 0, 0, 0)).x0;
}

/**
 *   @brief    Make a Host call
 *   @param    imm     - Immediate value
 *   @return   Returns command return status
**/
uint64_t val_realm_rsi_host_call(uint16_t imm)
{
     gv_realm_host_call.imm = imm;
     return (val_smc_call(RSI_HOST_CALL, (uint64_t)&gv_realm_host_call, 0, 0, 0, 0, 0, 0)).x0;
}

/**
 *   @brief    Request RIPAS of a target IPA range to be changed to a specified value.
 *   @param    base     - Base of target IPA region
 *   @param    size     - Size of target IPA region
 *   @param    ripas    - RIPAS value
 *   @return   Returns args, X0: Returns command return status,
 *                           X1: Base of IPA region which was notmodified by the command
**/
val_smc_param_ts val_realm_rsi_ipa_state_set(uint64_t base, uint64_t size, uint8_t ripas)
{
       val_smc_param_ts args;

       args = val_smc_call(RSI_IPA_STATE_SET, base, size, (uint64_t)ripas, 0, 0, 0, 0);

       return args;
}

/**
 *   @brief    Get RIPAS of a target page
 *   @param    addr     - IPA of target page
 *   @return   Returns args, X0: Returns command return status,
 *                           X1: RIPAS value
**/
val_smc_param_ts val_realm_rsi_ipa_state_get(uint64_t ipa_base)
{
       val_smc_param_ts args;

       args = val_smc_call(RSI_IPA_STATE_GET, ipa_base, 0, 0, 0, 0, 0, 0);

       return args;
}

/**
 *   @brief    Make a host call
 *   @param    imm     - Immediate value
 *   @return   Returns realm host call staructure
**/
val_realm_rsi_host_call_t *val_realm_rsi_host_call_ripas(uint16_t imm)
{
     gv_realm_host_call.imm = imm;

     val_smc_call(RSI_HOST_CALL, (uint64_t)&gv_realm_host_call, 0, 0, 0, 0, 0, 0);

     return &gv_realm_host_call;
}

/**
 *   @brief    Get realm IPA width
 *   @param    void
 *   @return   Returns realm IPA width
**/
uint64_t val_realm_get_ipa_width(void)
{
    val_realm_rsi_realm_config((uint64_t)realm_config_buff);
    return (uint64_t)*realm_config_buff;
}
