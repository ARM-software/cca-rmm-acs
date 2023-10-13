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
    return (val_smc_call(RSI_VERSION, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)).x0;
}

/**
 *   @brief    Read configuration for the current Realm
 *   @param    addr     - IPA of the Granule to which the configuration data will be written
 *   @return   Returns command return status
**/
uint64_t val_realm_rsi_realm_config(uint64_t buff)
{
    return (val_smc_call(RSI_REALM_CONFIG, buff, 0, 0, 0, 0, 0, 0, 0, 0, 0)).x0;
}

/**
 *   @brief    Make a Host call
 *   @param    imm     - Immediate value
 *   @return   Returns command return status
**/
uint64_t val_realm_rsi_host_call(uint16_t imm)
{
     gv_realm_host_call.imm = imm;
     return (val_smc_call(RSI_HOST_CALL, (uint64_t)&gv_realm_host_call,
                                        0, 0, 0, 0, 0, 0, 0, 0, 0)).x0;
}

/**
 *   @brief    Make a Host call to send data to Host
 *   @param    realm_host_params     - realm host call structure
 *   @return   Returns command return status
**/
uint64_t val_realm_rsi_host_params(val_realm_rsi_host_call_t *realm_host_params)
{
     realm_host_params->imm = VAL_SWITCH_TO_HOST;
     return (val_smc_call(RSI_HOST_CALL, (uint64_t)realm_host_params,
                                      0, 0, 0, 0, 0, 0, 0, 0, 0)).x0;
}

/**
 *   @brief    Request RIPAS of a target IPA range to be changed to a specified value.
 *   @param    base     - Base of target IPA region
 *   @param    size     - Size of target IPA region
 *   @param    ripas    - RIPAS value
 *   @param    flags    - Flags
 *   @return   Returns args, X0: Returns command return status,
 *                           X1: Base of IPA region which was notmodified by the command
**/
val_smc_param_ts val_realm_rsi_ipa_state_set(uint64_t base, uint64_t top,
                                           uint8_t ripas, uint64_t flags)
{
       val_smc_param_ts args;

       args = val_smc_call(RSI_IPA_STATE_SET, base, top, (uint64_t)ripas, flags, 0, 0, 0, 0, 0, 0);

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

       args = val_smc_call(RSI_IPA_STATE_GET, ipa_base, 0, 0, 0, 0, 0, 0, 0, 0, 0);

       return args;
}

/**
 *   @brief    Make a host call
 *   @param    imm     - Immediate value
 *   @return   Returns realm host call structure
**/
val_realm_rsi_host_call_t *val_realm_rsi_host_call_ripas(uint16_t imm)
{
     gv_realm_host_call.imm = imm;

     val_smc_call(RSI_HOST_CALL, (uint64_t)&gv_realm_host_call, 0, 0, 0, 0, 0, 0, 0, 0, 0);

     return &gv_realm_host_call;
}

/**
 *   @brief    Make a host call
 *   @param    imm     - Immediate value
 *   @return   Returns realm host call staructure
**/
uint64_t val_realm_rsi_host_call_struct(uint64_t gv_realm_host_call1)
{

    return  (val_smc_call(RSI_HOST_CALL, gv_realm_host_call1, 0, 0, 0, 0, 0, 0, 0, 0, 0)).x0;

}

/**
 *   @brief    Continue the operation to retrieve an attestation token.
 *   @param    addr     - IPA of the Granule to which the token will be written
 *   @return   Returns  args, X0: Returns command return status,
                              X1: Token size in bytes
**/
val_smc_param_ts val_realm_rsi_attestation_token_continue(uint64_t addr)
{
    val_smc_param_ts args;

    args = val_smc_call(RSI_ATTESTATION_TOKEN_CONTINUE, addr, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    return args;
}

/**
 *   @brief    Initialize the operation to retrieve an attestation token.
 *   @param    addr          - IPA of the Granule to which the token will be written
 *   @param    challenge_0   - Doubleword 0 of the challenge value
 *   @param    challenge_1   - Doubleword 1 of the challenge value
 *   @param    challenge_2   - Doubleword 2 of the challenge value
 *   @param    challenge_3   - Doubleword 3 of the challenge value
 *   @param    challenge_4   - Doubleword 4 of the challenge value
 *   @param    challenge_5   - Doubleword 5 of the challenge value
 *   @param    challenge_6   - Doubleword 6 of the challenge value
 *   @param    challenge_7   - Doubleword 7 of the challenge value
 *   @return   Returns command return status
**/
uint64_t val_realm_rsi_attestation_token_init(uint64_t addr, uint64_t challenge_0,
                 uint64_t challenge_1, uint64_t challenge_2, uint64_t challenge_3,
                 uint64_t challenge_4, uint64_t challenge_5, uint64_t challenge_6,
                                                            uint64_t challenge_7)
{
    return (val_smc_call(RSI_ATTESTATION_TOKEN_INIT, addr, challenge_0, challenge_1, challenge_2,
                         challenge_3, challenge_4, challenge_5, challenge_6, challenge_7, 0)).x0;
}

/**
 *   @brief    Extend Realm Extensible Measurement (REM) value.
 *   @param    index     - Measurement index
 *   @param    size      - Measurement size in bytes
 *   @param    value_0   - Doubleword 0 of the measurement value
 *   @param    value_1   - Doubleword 1 of the measurement value
 *   @param    value_2   - Doubleword 2 of the measurement value
 *   @param    value_3   - Doubleword 3 of the measurement value
 *   @param    value_4   - Doubleword 4 of the measurement value
 *   @param    value_5   - Doubleword 5 of the measurement value
 *   @param    value_6   - Doubleword 6 of the measurement value
 *   @param    value_7   - Doubleword 7 of the measurement value
 *   @return   Returns command return status
**/
uint64_t val_realm_rsi_measurement_extend(uint64_t index, uint64_t size, uint64_t value_0,
                                     uint64_t value_1, uint64_t value_2, uint64_t value_3,
                                     uint64_t value_4, uint64_t value_5, uint64_t value_6,
                                                                         uint64_t value_7)
{
    return (val_smc_call(RSI_MEASUREMENT_EXTEND, index, size, value_0, value_1, value_2, value_3,
                                                         value_4, value_5, value_6, value_7)).x0;
}

/**
 *   @brief    Read measurement for the current Realm.
 *   @param    index          - Measurement index
 *   @return   Returns  args, X0: Returns command return status,
 *                            X1: Doubleword 0 of the Realm measurement identified by index
 *                            X2: Doubleword 1 of the Realm measurement identified by index
 *                            X3: Doubleword 2 of the Realm measurement identified by index
 *                            X4: Doubleword 3 of the Realm measurement identified by index
 *                            X5: Doubleword 4 of the Realm measurement identified by index
 *                            X6: Doubleword 5 of the Realm measurement identified by index
 *                            X7: Doubleword 6 of the Realm measurement identified by index
 *                            X8: Doubleword 7 of the Realm measurement identified by index
**/
val_smc_param_ts val_realm_rsi_measurement_read(uint64_t index)
{
    val_smc_param_ts args;

    args = val_smc_call(RSI_MEASUREMENT_READ, index, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    return args;
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
