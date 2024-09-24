/*
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "val_realm_rsi.h"
#include "val_realm_framework.h"
#include "val_realm_planes.h"

__attribute__((aligned (PAGE_SIZE))) static uint8_t realm_config_buff[PAGE_SIZE];

__attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t gv_realm_host_call;

/**
 *   @brief    Returns RSI version
 *   @param    req    - Requested interface version.
 *   @param    output - Pointer to store lower/higher implemented interface version.
 *   @return   Returns command return status
**/

uint64_t val_realm_rsi_version(uint64_t req, val_realm_rsi_version_ts *output)

{
    val_smc_param_ts args;

    args = (val_smc_call(RSI_VERSION, req, 0, 0, 0, 0, 0, 0, 0, 0, 0));

    output->lower = args.x1;
    output->higher = args.x2;

    return args.x0;

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
 *                           X2: response - whether the host accepted or rejected the request
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
 *   @param    offset   - Offset within Granule to start of buffer in bytes
 *   @param    size     - Size of buffer in bytes
 *   @param    len      - Pointer to store number of bytes written to buffer
 *   @return   Returns command return status
**/
val_smc_param_ts val_realm_rsi_attestation_token_continue(uint64_t addr, uint64_t offset,
                                                            uint64_t size, uint64_t *len)
{
    val_smc_param_ts args;

    args = val_smc_call(RSI_ATTESTATION_TOKEN_CONTINUE, addr, offset, size, 0, 0, 0, 0, 0, 0, 0);

    *len = args.x1;
    return args;
}
/**
 *   @brief    Initialize the operation to retrieve an attestation token.
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
val_smc_param_ts val_realm_rsi_attestation_token_init(uint64_t challenge_0, uint64_t challenge_1,
                                              uint64_t challenge_2, uint64_t challenge_3,
                                              uint64_t challenge_4, uint64_t challenge_5,
                                              uint64_t challenge_6, uint64_t challenge_7)
{
    val_smc_param_ts args;
    args = val_smc_call(RSI_ATTESTATION_TOKEN_INIT, challenge_0, challenge_1, challenge_2,
                challenge_3, challenge_4, challenge_5, challenge_6, challenge_7, 0, 0);
    return args;
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
    if (val_realm_in_p0()) {
        val_realm_rsi_realm_config((uint64_t)realm_config_buff);
        return (uint64_t)*realm_config_buff;
    } else {
        return val_realm_psi_realm_config((uint64_t)realm_config_buff);
    }
}

/**
 *   @brief    Read feature register
 *   @param    index    -  Feature register index
 *   @param    value    -  Pointer to store Feature register value
 *   @return   Returns command return status
**/
uint64_t val_realm_rsi_features(uint64_t index, uint64_t *value)
{
    val_smc_param_ts args;

    args = val_smc_call(RSI_FEATURES, index, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    *value = args.x1;
    return args.x0;
}

/**
 *   @brief    Read overlay permission value.
 *   @param    plane_index    -  Plane index
 *   @param    perm_index     -  Permission index
 *   @param    value          -  Pointer to store permission value
 *   @return   SMC return arguments
**/
val_smc_param_ts val_realm_rsi_mem_get_perm_value(uint64_t plane_index, uint64_t perm_index)
{
    return val_smc_call(RSI_MEM_GET_PERM_VALUE, plane_index, perm_index, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Set overlay permission index for a specified IPA range.
 *   @param    base            -  Base of target IPA region.
 *   @param    top             -  Top of target IPA region.
 *   @param    perm_index      -  Permission index.
 *   @param    cookie          -  Cookie value.
 *   @return   SMC return arguments
**/
val_smc_param_ts val_realm_rsi_mem_set_perm_index(uint64_t base, uint64_t top,  uint64_t perm_index,
                                                                                    uint64_t cookie)
{
    return val_smc_call(RSI_MEM_SET_PERM_INDEX, base, top, perm_index, cookie, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Set overlay permission value.
 *   @param    plane_index    -  Plane index
 *   @param    perm_index     -  Permission index
 *   @param    value          -  Memory permission value
 *   @return   SMC return arguments
**/
val_smc_param_ts val_realm_rsi_mem_set_perm_value(uint64_t plane_index, uint64_t perm_index,
                                                                                   uint64_t value)
{
    return val_smc_call(RSI_MEM_SET_PERM_VALUE, plane_index, perm_index, value, 0, 0, 0,
                                                                                0, 0, 0, 0);
}

/**
 *   @brief    Enter a plane.
 *   @param    plane_idx    -  Plane index
 *   @param    run_ptr        -  Pointer to PlaneRun object.
 *   @return   SMC return arguments
**/
val_smc_param_ts val_realm_rsi_plane_enter(uint64_t plane_idx, uint64_t run_ptr)
{
    return val_smc_call(RSI_PLANE_ENTER, plane_idx, run_ptr, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Read a plane register.
 *   @param    plane_idx    - Plane index
 *   @param    encoding     - Encoding of target register
 *   @return   SMC return arguments
**/
val_smc_param_ts val_realm_rsi_plane_reg_read(uint64_t plane_idx, uint64_t encoding)
{
    return val_smc_call(RSI_PLANE_REG_READ, plane_idx, encoding, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Write a plane register.
 *   @param    plane_idx    - Plane index
 *   @param    encoding     - Encoding of target register
 *   @param    value        - Value to write to target register
 *   @return   SMC return arguments
**/
val_smc_param_ts val_realm_rsi_plane_reg_write(uint64_t plane_idx, uint64_t encoding,
                                                                             uint64_t value)
{
    return val_smc_call(RSI_PLANE_REG_WRITE, plane_idx, encoding, value, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Continue an interruptible Realm device operation.
 *   @param    dev_id    -  Realm device identifier
 *   @return   SMC return arguments
**/
val_smc_param_ts val_realm_rsi_rdev_continue(uint64_t dev_id)
{
    return val_smc_call(RSI_RDEV_CONTINUE, dev_id, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Get digests of Realm device attestation evidence.
 *   @param    dev_id    -  Realm device identifier
 *   @param    addr      -  IPA of the Granule to which the digests will be written
 *   @return   SMC return arguments
**/
val_smc_param_ts val_realm_rsi_rdev_get_digests(uint64_t dev_id, uint64_t addr)
{
    return val_smc_call(RSI_RDEV_GET_DIGESTS, dev_id, addr, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Get Realm device interface report.
 *   @param    dev_id         -  Realm device identifier
 *   @param    version_max    -  Maximum TDISP version accepted by caller
 *   @return   SMC return arguments
**/
val_smc_param_ts val_realm_rsi_rdev_get_interface_report(uint64_t dev_id, uint64_t version_max)
{
    return val_smc_call(RSI_RDEV_GET_INTERFACE_REPORT, dev_id, version_max, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Get Realm device measurements.
 *   @param    dev_id      -  Realm device identifier
 *   @param    params_ptr  -  IPA of measurement parameters
 *   @return   SMC return arguments
**/
val_smc_param_ts val_realm_rsi_rdev_get_measurements(uint64_t dev_id, uint64_t params_ptr)
{
    return val_smc_call(RSI_RDEV_GET_MEASUREMENTS, dev_id, params_ptr, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Get state of a Realm device.
 *   @param    dev_id         -  Realm device identifier
 *   @return   SMC return arguments
**/
val_smc_param_ts val_realm_rsi_rdev_get_state(uint64_t dev_id)
{
   return val_smc_call(RSI_RDEV_GET_INTERFACE_REPORT, dev_id, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Lock a Realm device.
 *   @param    dev_id         -  Realm device identifier
 *   @return   SMC return arguments
**/
val_smc_param_ts val_realm_rsi_rdev_lock(uint64_t dev_id)
{
    return val_smc_call(RSI_RDEV_LOCK, dev_id, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Start a Realm device.
 *   @param    dev_id         -  Realm device identifier
 *   @return   SMC return arguments
**/
val_smc_param_ts val_realm_rsi_rdev_start(uint64_t dev_id)
{
    return val_smc_call(RSI_RDEV_START, dev_id, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Stop a Realm device.
 *   @param    dev_id         -  Realm device identifier
 *   @return   SMC return arguments
**/
val_smc_param_ts val_realm_rsi_rdev_stop(uint64_t dev_id)
{
    return val_smc_call(RSI_RDEV_STOP, dev_id, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Validate MMIO mappings for a Realm device.
 *   @param    dev_id         -  Realm device identifier
 *   @param    ipa_base       -  Base of target IPA region
 *   @param    ipa_top        -  Top of target IPA region
 *   @param    pa_base        -  Base of target PA region
 *   @param    flags          -  Flags
 *   @return   SMC return arguments
**/
val_smc_param_ts val_realm_rsi_rdev_validate_io(uint64_t dev_id, uint64_t ipa_base,
                                uint64_t ipa_top, uint64_t pa_base, uint64_t flags)
{
    return val_smc_call(RSI_RDEV_VALIDATE_IO, dev_id, ipa_base, ipa_top, pa_base, flags,
                                                                         0, 0, 0, 0, 0);
}
