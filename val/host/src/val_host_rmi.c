/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_rmi.h"
#include "val_libc.h"
#include "val_host_realm.h"

/**
 *   @brief    Returns RMI version
 *   @param    req    - Requested interface version.
 *   @param    output - Pointer to store lower/higher implemented interface version.
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_version(uint64_t req, val_host_rmi_version_ts *output)
{
    val_smc_param_ts args;

    args = (val_smc_call(RMI_VERSION, req, 0, 0, 0, 0, 0, 0, 0, 0, 0));

    output->lower = args.x1;
    output->higher = args.x2;

    return args.x0;
}

/**
 *   @brief    Read feature register
 *   @param    index    -  Feature register index
 *   @param    value    -  Pointer to store Feature register value
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_features(uint64_t index, uint64_t *value)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_FEATURES, index, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    *value = args.x1;
    return args.x0;
}

/**
 *   @brief    Get number of auxiliary Granules required for a REC
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    aux_count    -  Pointer to store number of auxiliary Granules required for a REC
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_rec_aux_count(uint64_t rd, uint64_t *aux_count)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_REC_AUX_COUNT, rd, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    *aux_count = args.x1;
    return args.x0;
}

/**
 *   @brief    Creates a Data Granule, copying contents
 *             from a Non-secure Granule provided by the caller
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    data         -  PA of the target data
 *   @param    ipa          -  IPA at which the Granule will be mapped in the target Realm
 *   @param    src          -  PA of the source Granule
 *   @param    flags        -  Flags
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_data_create(uint64_t rd, uint64_t data,
               uint64_t ipa, uint64_t src, uint64_t flags)
{
    uint64_t ret;

    ret = (val_smc_call(RMI_DATA_CREATE, rd, data, ipa, src, flags, 0, 0, 0, 0, 0)).x0;
    if (ret)
    {
        return ret;
    }
    val_host_update_granule_state(rd, GRANULE_DATA, data, ipa, 0, 0);
    return ret;

}

/**
 *   @brief    Creates a Data Granule with unknown contents
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    data         -  PA of the target data
 *   @param    ipa          -  IPA at which the Granule will be mapped in the target Realm
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_data_create_unknown(uint64_t rd,
                   uint64_t data, uint64_t ipa)
{
    uint64_t ret;

    ret = (val_smc_call(RMI_DATA_CREATE_UNKNOWN, rd, data, ipa, 0, 0, 0, 0, 0, 0, 0)).x0;
    if (ret)
    {
        return ret;
    }
    val_host_update_granule_state(rd, GRANULE_DATA, data, ipa, 0, 0);
    return ret;

}

/**
 *   @brief    Delegates a Granule.
 *   @param    addr         -  PA of the target Granule
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_granule_delegate(uint64_t addr)
{
    uint64_t ret;

    ret = (val_smc_call(RMI_GRANULE_DELEGATE, addr, 0, 0, 0, 0, 0, 0, 0, 0, 0)).x0;
    if (ret)
    {
        return ret;
    }

    val_host_add_granule(GRANULE_DELEGATED, addr, NULL);

    return ret;
}

/**
 *   @brief    Undelegates a Granule
 *   @param    addr         -  PA of the target Granule
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_granule_undelegate(uint64_t addr)
{
    uint64_t ret;

    ret = (val_smc_call(RMI_GRANULE_UNDELEGATE, addr, 0, 0, 0, 0, 0, 0, 0, 0, 0)).x0;
    if (ret)
    {
        return ret;
    }
    val_host_update_destroy_granule_state(0, addr, 0, 0, GRANULE_UNDELEGATED, 0, 0);
    return ret;
}

/**
 *   @brief    Completes a pending PSCI command which was called with an MPIDR argument,
 *             by providing the corresponding REC
 *   @param    calling_rec         -  PA of the calling REC
 *   @param    target_rec          -  PA of the target REC
 *   @param    status              -  Status of the PSCI request
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_psci_complete(uint64_t calling_rec,
                 uint64_t target_rec, uint64_t status)
{
    return (val_smc_call(RMI_PSCI_COMPLETE, calling_rec, target_rec, status,
                                                       0, 0, 0, 0, 0, 0, 0)).x0;
}

/**
 *   @brief    Activates a Realm
 *   @param    rd         -  PA of the RD
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_realm_activate(uint64_t rd)
{
    return (val_smc_call(RMI_REALM_ACTIVATE, rd, 0, 0, 0, 0, 0, 0, 0, 0, 0)).x0;
}

/**
 *   @brief    Creates a Realm
 *   @param    rd              -  PA of the RD
 *   @param    params_ptr      -  PA of Realm parameters
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_realm_create(uint64_t rd, uint64_t params_ptr)
{
    uint64_t ret;

    ret = (val_smc_call(RMI_REALM_CREATE, rd, params_ptr, 0, 0, 0, 0, 0, 0, 0, 0)).x0;
    if (ret)
    {
        return ret;
    }
    val_host_update_granule_state(rd, GRANULE_RD, rd, 0, 0, 0);
    return ret;
}

/**
 *   @brief    Destroys a Realm
 *   @param    rd         -  PA of the RD
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_realm_destroy(uint64_t rd)
{
    uint64_t ret;

    ret = (val_smc_call(RMI_REALM_DESTROY, rd, 0, 0, 0, 0, 0, 0, 0, 0, 0)).x0;
    if (ret)
    {
        return ret;
    }
    val_host_update_destroy_granule_state(rd, rd, 0, 0, GRANULE_DELEGATED, GRANULE_RD, 0);
    return ret;
}

/**
 *   @brief    Destroys a Data Granule
 *   @param    rd              -  PA of the RD which owns the target Data
 *   @param    ipa             -  IPA at which the Granule is mapped in the target Realm
 *   @param    data_destroy    -  Pointer to store output values for given ABI
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_data_destroy(uint64_t rd, uint64_t ipa,
                      val_host_data_destroy_ts *data_destroy)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_DATA_DESTROY, rd, ipa, 0, 0, 0, 0, 0, 0, 0, 0);
    data_destroy->data = args.x1;
    data_destroy->top = args.x2;

    if (args.x0)
    {
        return args.x0;
    }
    val_host_update_destroy_granule_state(rd, 0, ipa, 0, GRANULE_DELEGATED, GRANULE_DATA, 0);
    return args.x0;
}

/**
 *   @brief    Creates a REC
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    rec          -  PA of the target REC
 *   @param    params_ptr   -  PA of REC parameters
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_rec_create(uint64_t rd, uint64_t rec,
                 uint64_t params_ptr)
{
    uint64_t ret;

    ret = (val_smc_call(RMI_REC_CREATE, rd, rec, params_ptr, 0, 0, 0, 0, 0, 0, 0)).x0;
    if (ret)
    {
        return ret;
    }
    val_host_update_granule_state(rd, GRANULE_REC, rec, 0, 0, 0);
    return ret;
}

/**
 *   @brief    Destroys a REC
 *   @param    rec          -  PA of the target REC
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_rec_destroy(uint64_t rec)
{
    uint64_t ret;

    ret = (val_smc_call(RMI_REC_DESTROY, rec, 0, 0, 0, 0, 0, 0, 0, 0, 0)).x0;
    if (ret)
    {
        return ret;
    }
    val_host_update_destroy_granule_state(0, rec, 0, 0, GRANULE_DELEGATED, GRANULE_REC, 0);
    return ret;
}

/**
 *   @brief    Enter a REC
 *   @param    rec          -  PA of the target REC
 *   @param    run_ptr      -  PA of RecRun object
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_rec_enter(uint64_t rec, uint64_t run_ptr)
{
    val_host_rec_run_ts *run = (val_host_rec_run_ts *)run_ptr;
    val_host_rec_enter_flags_ts rec_enter_flags;
    uint64_t ret;

rec_enter:
    ret = (val_smc_call(RMI_REC_ENTER, rec, run_ptr, 0, 0, 0, 0, 0, 0, 0, 0)).x0;

    /* In case of realm exit due to hvc print functionality,
     * re-enter rec after printing the realm message onto
     * console.
     */
    if (!ret &&
        (run->exit.exit_reason == RMI_EXIT_HOST_CALL) &&
        (run->exit.imm == VAL_REALM_PRINT_MSG))
    {
        rec_enter_flags.emul_mmio = 0;
        rec_enter_flags.inject_sea = 0;
        val_memcpy(&run->enter.flags, &rec_enter_flags, sizeof(rec_enter_flags));
        val_host_realm_printf_msg_service();
        goto rec_enter;
    }

    return ret;
}
/**
 *   @brief    Handles Relam S2 Permission change request
 *   @param    rd              -  PA of the RD for the target Realm
 *   @param    rec             -  PA of the target REC
 *   @param    base            -  Base IPA
 *   @param    top             -  Top IPA
 *   @param    *progress_addr  -  Pointer to progress address
 *   @param    *rtt_tree       -  Pointr to RTT tree
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_set_s2ap(uint64_t rd, uint64_t rec, uint64_t base, uint64_t top)
{
    return val_smc_call(RMI_RTT_SET_S2AP, rd, rec, base, top, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Creates an RTT
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    rtt          -  PA of the target RTT
 *   @param    ipa          -  Base of the IPA range described by the RTT
 *   @param    level        -  RTT level
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_rtt_create(uint64_t rd, uint64_t rtt,
              uint64_t ipa, uint64_t level)
{
    uint64_t ret;

    ret = (val_smc_call(RMI_RTT_CREATE, rd, rtt, ipa, level, 0, 0, 0, 0, 0, 0)).x0;
    if (ret)
    {
        return ret;
    }
    val_host_update_granule_state(rd, GRANULE_RTT, rtt, ipa, level, 0);
    return ret;
}

/**
 *   @brief    Destroys a homogeneous RTT
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    ipa          -  Base of the IPA range described by the RTT
 *   @param    level        -  RTT level
 *   @param    rtt          -  Pointer to store PA of the RTT which was destroyed
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_rtt_fold(uint64_t rd,
              uint64_t ipa, uint64_t level, uint64_t *rtt)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_RTT_FOLD, rd, ipa, level, 0, 0, 0, 0, 0, 0, 0);
    if (args.x0)
    {
        return args.x0;
    }

    *rtt = args.x1;

    val_host_update_destroy_granule_state(rd, 0, ipa, level, GRANULE_DELEGATED, GRANULE_RTT, 0);
    return args.x0;
}

/**
 *   @brief    Destroys an RTT
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    ipa          -  Base of the IPA range described by the RTT
 *   @param    level        -  RTT level
 *   @param    rtt_destroy  -  Pointer to store output values for given ABI
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_rtt_destroy(uint64_t rd,
               uint64_t ipa, uint64_t level, val_host_rtt_destroy_ts *rtt_destroy)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_RTT_DESTROY, rd, ipa, level, 0, 0, 0, 0, 0, 0, 0);
    rtt_destroy->rtt = args.x1;
    rtt_destroy->top = args.x2;

    if (args.x0)
    {
        return args.x0;
    }

    val_host_update_destroy_granule_state(rd, 0, ipa, level, GRANULE_DELEGATED, GRANULE_RTT, 0);
    return args.x0;
}

/**
 *   @brief    Creates a mapping from an Unprotected IPA to a Non-secure PA
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    ipa          -  IPA at which the Granule will be mapped in the target Realm
 *   @param    level        -  RTT level
 *   @param    desc         -  RTTE descriptor
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_rtt_map_unprotected(uint64_t rd, uint64_t ipa,
                  uint64_t level, uint64_t desc)
{
    uint64_t ret;

    ret = (val_smc_call(RMI_RTT_MAP_UNPROTECTED, rd, ipa, level, desc, 0, 0, 0, 0, 0, 0)).x0;
    if (ret)
    {
        return ret;
    }
    val_host_update_granule_state(rd, GRANULE_UNPROTECTED, ipa, ipa, level, 0);
    return ret;
}

/**
 *   @brief    Reads an RTTE
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    ipa          -  Realm Address for which to read the RTTE
 *   @param    level        -  RTT level at which to read the RTTE
 *   @param    rtt          -  Pointer to store output of given ABI
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_rtt_read_entry(uint64_t rd, uint64_t ipa,
                 uint64_t level, val_host_rtt_entry_ts *rtt)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_RTT_READ_ENTRY, rd, ipa, level, 0, 0, 0, 0, 0, 0, 0);

    rtt->walk_level = args.x1;
    rtt->state = args.x2;
    rtt->desc = args.x3;
    rtt->ripas = args.x4;

    return args.x0;
}

/**
 *   @brief    Removes a mapping at an Unprotected IPA
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    ipa          -  IPA at which the Granule is mapped in the target Realm
 *   @param    level        -  RTT level
 *   @param    top          -  Pointer to store Top IPA of non-live RTT entries,
 *                             from entry at which the RTT walk terminated
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_rtt_unmap_unprotected(uint64_t rd, uint64_t ipa,
                    uint64_t level, uint64_t *top)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_RTT_UNMAP_UNPROTECTED, rd, ipa, level, 0, 0, 0, 0, 0, 0, 0);
    *top = args.x1;

    if (args.x0)
    {
        return args.x0;
    }
    val_host_update_destroy_granule_state(rd, ipa, 0, 0, GRANULE_UNPROTECTED,
                                                         GRANULE_UNPROTECTED, 0);
    return args.x0;
}

/**
 *   @brief    Set the RIPAS of a target IPA range to RAM, for a Realm in the NEW state
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    base         -  Base of target IPA region
 *   @param    top          -  Top of target IPA region
 *   @param    out_top      -  Pointer to store the Top IPA of range whose RIPAS was modified
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_rtt_init_ripas(uint64_t rd, uint64_t base,
                                uint64_t top, uint64_t *out_top)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_RTT_INIT_RIPAS, rd, base, top, 0, 0, 0, 0, 0, 0, 0);

    *out_top = args.x1;
    return args.x0;
}

/**
 *   @brief    Completes a request made by the Realm to change the RIPAS of a target IPA range
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    rec          -  PA of the target REC
 *   @param    base         -  Base of target IPA region
 *   @param    top          -  Top of target IPA region
 *   @param    out_top      -  Pointer to store the Top IPA of range whose RIPAS was modified
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_rtt_set_ripas(uint64_t rd, uint64_t rec,
                    uint64_t base, uint64_t top, uint64_t *out_top)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_RTT_SET_RIPAS, rd, rec, base, top, 0, 0, 0, 0, 0, 0);

    *out_top = args.x1;
    return args.x0;
}

/**
 *   @brief    Creates an Auxiliary RTT
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    rtt          -  PA of the target RTT
 *   @param    ipa          -  Base of the IPA range described by the RTT
 *   @param    level        -  RTT level
 *   @param    index        -  RTT index
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_aux_create(uint64_t rd, uint64_t rtt,
              uint64_t ipa, uint64_t level, uint64_t index)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_RTT_AUX_CREATE, rd, rtt, ipa, level, index, 0, 0, 0, 0, 0);

    if (args.x0)
        return args;

    val_host_update_granule_state(rd, GRANULE_RTT_AUX, rtt, ipa, level, index);

    return args;
}

/**
 *   @brief    Destroys an Auxiliary RTT
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    ipa          -  Base of the IPA range described by the RTT
 *   @param    level        -  RTT level
 *   @param    index        -  RTT index
 *   @param    rtt_destroy  -  Pointer to store output values for given ABI
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_aux_destroy(uint64_t rd,
                            uint64_t ipa, uint64_t level, uint64_t index)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_RTT_AUX_DESTROY, rd, ipa, level, index, 0, 0, 0, 0, 0, 0);

    if (args.x0)
        return args;

    val_host_update_destroy_granule_state(rd, 0, ipa, level, GRANULE_DELEGATED,
                                                             GRANULE_RTT_AUX, index);

    return args;
}

/**
 *   @brief    Destroys a homogeneous auxiliary RTT
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    ipa          -  Base of the IPA range described by the RTT
 *   @param    level        -  RTT level
 *   @param    index        -  RTT index
 *   @param    rtt          -  Pointer to store PA of the RTT which was destroyed
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_aux_fold(uint64_t rd,
                                uint64_t ipa, uint64_t level, uint64_t index)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_RTT_AUX_FOLD, rd, ipa, level, index, 0, 0, 0, 0, 0, 0);

    if (args.x0)
        return args;

    val_host_update_destroy_granule_state(rd, 0, ipa, level, GRANULE_DELEGATED,
                                                             GRANULE_RTT_AUX, index);
    return args;
}

/**
 *   @brief    Creates a auxiliary mapping from an Protected IPA to a Non-secure PA
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    ipa          -  IPA at which the Granule will be mapped in the target Realm
 *   @param    index        -  RTT index
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_aux_map_protected(uint64_t rd, uint64_t ipa, uint64_t index)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_RTT_AUX_MAP_PROTECTED, rd, ipa, index, 0, 0, 0, 0, 0, 0, 0);

    if (args.x0)
        return args;

    val_host_update_aux_rtt_info(GRANULE_DATA, rd, index, ipa, true);

    return args;
}

/**
 *   @brief    Creates a auxiliary mapping from an Unprotected IPA to a Non-secure PA
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    ipa          -  IPA at which the Granule will be mapped in the target Realm
 *   @param    index        -  RTT index
 *   @return   Returns command return status
**/
val_smc_param_ts val_host_rmi_rtt_aux_map_unprotected(uint64_t rd, uint64_t ipa, uint64_t index)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_RTT_AUX_MAP_UNPROTECTED, rd, ipa, index, 0, 0, 0, 0, 0, 0, 0);

    if (args.x0)
        return args;

    val_host_update_aux_rtt_info(GRANULE_UNPROTECTED, rd, index, ipa, true);

    return args;
}

/**
 *   @brief    Removes a auxiliary mapping at an protected IPA
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    ipa          -  IPA at which the Granule is mapped in the target Realm
 *   @param    index        -  RTT index
 *   @param    top          -  Pointer to store Top IPA of non-live RTT entries,
 *                             from entry at which the RTT walk terminated
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_aux_unmap_protected(uint64_t rd, uint64_t ipa, uint64_t index)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_RTT_AUX_UNMAP_PROTECTED, rd, ipa, index, 0, 0, 0, 0, 0, 0, 0);

    if (args.x0)
        return args;

    val_host_update_aux_rtt_info(GRANULE_DATA, rd, index, ipa, false);

    return args;
}

/**
 *   @brief    Removes a mapping at an Unprotected IPA
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    ipa          -  IPA at which the Granule is mapped in the target Realm
 *   @param    level        -  RTT level
 *   @param    top          -  Pointer to store Top IPA of non-live RTT entries,
 *                             from entry at which the RTT walk terminated
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_aux_unmap_unprotected(uint64_t rd, uint64_t ipa, uint64_t index)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_RTT_AUX_UNMAP_UNPROTECTED, rd, ipa, index, 0, 0, 0, 0, 0, 0, 0);

    if (args.x0)
        return args;

    val_host_update_aux_rtt_info(GRANULE_UNPROTECTED, rd, index, ipa, false);

    return args;
}

/**
 *   @brief    Delegate an IO Granule.
 *   @param    addr         -  PA of the target Granule
 *   @param    flags        -  Flags
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_granule_io_delegate(uint64_t addr, uint64_t flags)
{
    return val_smc_call(RMI_GRANULE_IO_DELEGATE, addr, flags, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Undelegate an IO Granule.
 *   @param    addr         -  PA of the target Granule
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_granule_io_undelegate(uint64_t addr)
{
    return val_smc_call(RMI_GRANULE_IO_UNDELEGATE, addr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Creates an IO Granule.
 *   @param    rd         -  PA of the RD for the target Realm
 *   @param    ipa        -  IPA at which the Granule will be mapped in the target Realm
 *   @param    flags      -  Flags
 *   @param    desc       -  RTTE descriptor
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_io_create(uint64_t rd, uint64_t ipa, uint64_t flags, uint64_t desc)
{
    return val_smc_call(RMI_IO_CREATE, rd, ipa, flags, desc, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Destroys an IO Granule.
 *   @param    rd         -  PA of the RD which owns the target IO Granule
 *   @param    ipa        -  IPA at which the Granule is mapped in the target Realm
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_io_destroy(uint64_t rd, uint64_t ipa)
{
    return val_smc_call(RMI_IO_DESTROY, rd, ipa, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Abort device communication associated with a PDEV.
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_pdev_abort(uint64_t pdev_ptr)
{
    return val_smc_call(RMI_PDEV_ABORT, pdev_ptr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Perform device communication associated with a PDEV.
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @param    data_ptr         -  PA of the communication data structure
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_pdev_communicate(uint64_t pdev_ptr, uint64_t data_ptr)
{
    return val_smc_call(RMI_PDEV_COMMUNICATE, pdev_ptr, data_ptr, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Create a PDEV.
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @param    params_ptr       -  PA of PDEV parameters
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_pdev_create(uint64_t pdev_ptr, uint64_t params_ptr)
{
    return val_smc_call(RMI_PDEV_CREATE, pdev_ptr, params_ptr, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Destroy a PDEV
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_pdev_destroy(uint64_t pdev_ptr)
{
    return val_smc_call(RMI_PDEV_DESTROY, pdev_ptr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Get state of a PDEV
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_pdev_get_state(uint64_t pdev_ptr)
{
    return val_smc_call(RMI_PDEV_GET_STATE, pdev_ptr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Reset the IDE link of a PDEV.
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_pdev_ide_reset(uint64_t pdev_ptr)
{
    return val_smc_call(RMI_PDEV_IDE_RESET, pdev_ptr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Notify the RMM of an event related to a PDEV.
 *   @param    pdev_ptr    -  PA of the PDEV
 *   @param    ev          -  Event type
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_pdev_notify(uint64_t pdev_ptr, uint64_t ev)
{
    return val_smc_call(RMI_PDEV_NOTIFY, pdev_ptr, ev, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Provide public key associated with a PDEV.
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @param    key              -  PA of the key
 *   @param    len              -  Length of the key in bytes
 *   @param    algo             -  Signature algorithm
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_pdev_set_key(uint64_t pdev_ptr, uint64_t key,
                                                uint64_t len, uint8_t algo)
{
    return val_smc_call(RMI_PDEV_SET_KEY, pdev_ptr, key, len, algo, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Stop a PDEV.
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_pdev_stop(uint64_t pdev_ptr)
{
    return val_smc_call(RMI_PDEV_STOP, pdev_ptr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Abort device communication associated with a VDEV.
 *   @param    vdev_ptr         -  PA of the VDEV
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_vdev_abort(uint64_t vdev_ptr)
{
    return val_smc_call(RMI_VDEV_ABORT, vdev_ptr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Perform device communication associated with a VDEV.
 *   @param    vdev_ptr         -  PA of the VDEV
 *   @param    data_ptr         -  PA of the communication data structure
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_vdev_communicate(uint64_t vdev_ptr, uint64_t data_ptr)
{
    return val_smc_call(RMI_VDEV_COMMUNICATE, vdev_ptr, data_ptr, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Create a VDEV.
 *   @param    rd               -  PA of the RD
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @param    vdev_ptr         -  PA of the VDEV
 *   @param    params_ptr       -  PA of VDEV parameters
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_vdev_create(uint64_t rd, uint64_t pdev_ptr,
                                  uint64_t vdev_ptr, uint64_t params_ptr)
{
    return val_smc_call(RMI_VDEV_CREATE, rd, pdev_ptr, vdev_ptr, params_ptr, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Destroy a VDEV.
 *   @param    vdev_ptr         -  PA of the VDEV
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_vdev_destroy(uint64_t vdev_ptr)
{
    return val_smc_call(RMI_VDEV_DESTROY, vdev_ptr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Get state of a VDEV.
 *   @param    vdev_ptr         -  PA of the VDEV
 *   @return   SMC return arguments
 *                     X1 state  - VDEV state
**/
val_smc_param_ts val_host_rmi_vdev_get_state(uint64_t vdev_ptr)
{
    return val_smc_call(RMI_VDEV_GET_STATE, vdev_ptr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Stop a VDEV.
 *   @param    vdev_ptr         -  PA of the VDEV
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_vdev_stop(uint64_t vdev_ptr)
{
    return val_smc_call(RMI_VDEV_STOP, vdev_ptr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Change state of a MEC to Private
 *   @param    mecid         -  MECID
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_mec_set_private(uint64_t mecid)
{
    return val_smc_call(RMI_MEC_SET_PRIVATE, mecid, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Change state of a MEC to Shared
 *   @param    mecid         -  MECID
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_mec_set_shared(uint64_t mecid)
{
    return val_smc_call(RMI_MEC_SET_SHARED, mecid, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}
