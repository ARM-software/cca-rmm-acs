/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
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
 *   @brief    Completes a pending PSCI command
 *   @param    calling_rec         -  PA of the calling REC
 *   @param    status              -  Status of the PSCI request
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_psci_complete(uint64_t calling_rec, uint64_t status)
{
    return (val_smc_call(RMI_PSCI_COMPLETE, calling_rec, status, 0,
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
val_smc_param_ts val_host_rmi_realm_create_smc(uint64_t rd, uint64_t params_ptr)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_REALM_CREATE, rd, params_ptr, 0, 0, 0, 0, 0, 0, 0, 0);
    if (RMI_STATUS(args.x0) == RMI_SUCCESS)
        val_host_update_granule_state(rd, GRANULE_RD, rd, 0, 0, 0);

    return args;
}

/**
 *   @brief    Destroys a Realm
 *   @param    rd         -  PA of the RD
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_realm_destroy_smc(uint64_t rd)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_REALM_DESTROY, rd, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (args.x0)
        return args;

    val_host_update_destroy_granule_state(rd, rd, 0, 0, GRANULE_DELEGATED, GRANULE_RD, 0);
    return args;
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
 *   @brief    Creates a REC and returns the raw RMI output registers
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    rec          -  PA of the target REC
 *   @param    params_ptr   -  PA of REC parameters
 *   @return   Returns raw RMI output registers
**/
val_smc_param_ts val_host_rmi_rec_create_smc(uint64_t rd, uint64_t rec,
                 uint64_t params_ptr)
{
    return val_smc_call(RMI_REC_CREATE, rd, rec, params_ptr, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Destroys a REC
 *   @param    rec          -  PA of the target REC
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rec_destroy_smc(uint64_t rec)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_REC_DESTROY, rec, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    if (args.x0)
        return args;

    val_host_update_destroy_granule_state(0, rec, 0, 0, GRANULE_DELEGATED, GRANULE_REC, 0);
    return args;
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
    val_host_rec_enter_flags_ts rec_enter_flags = {0};
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
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_create_smc(uint64_t rd, uint64_t rtt,
                  uint64_t ipa, uint64_t level)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_RTT_CREATE, rd, rtt, ipa, level, 0, 0, 0, 0, 0, 0);
    if (args.x0)
        return args;

    val_host_update_granule_state(rd, GRANULE_RTT, rtt, ipa, level, 0);
    return args;
}

/**
 *   @brief    Destroys a homogeneous RTT
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    ipa          -  Base of the IPA range described by the RTT
 *   @param    level        -  RTT level
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_fold_smc(uint64_t rd,
                                           uint64_t ipa, uint64_t level)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_RTT_FOLD, rd, ipa, level, 0, 0, 0, 0, 0, 0, 0);
    if (args.x0)
        return args;

    val_host_update_destroy_granule_state(rd, 0, ipa, level, GRANULE_DELEGATED, GRANULE_RTT, 0);
    return args;
}

/**
 *   @brief    Destroys an RTT
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    ipa          -  Base of the IPA range described by the RTT
 *   @param    level        -  RTT level
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_destroy_smc(uint64_t rd,
                                               uint64_t ipa, uint64_t level)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_RTT_DESTROY, rd, ipa, level, 0, 0, 0, 0, 0, 0, 0);
    if (args.x0)
        return args;

    val_host_update_destroy_granule_state(rd, 0, ipa, level, GRANULE_DELEGATED, GRANULE_RTT, 0);
    return args;
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
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_init_ripas_smc(uint64_t rd, uint64_t base,
                                    uint64_t top, uint64_t *out_top)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_RTT_INIT_RIPAS, rd, base, top, 0, 0, 0, 0, 0, 0, 0);

    *out_top = args.x1;
    return args;
}

/**
 *   @brief    Completes a request made by the Realm to change the RIPAS of a target IPA range
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    rec          -  PA of the target REC
 *   @param    base         -  Base of target IPA region
 *   @param    top          -  Top of target IPA region
 *   @param    out_top      -  Pointer to store the Top IPA of range whose RIPAS was modified
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_set_ripas_smc(uint64_t rd, uint64_t rec,
                    uint64_t base, uint64_t top, uint64_t *out_top)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_RTT_SET_RIPAS, rd, rec, base, top, 0, 0, 0, 0, 0, 0);

    *out_top = args.x1;
    return args;
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
 *   @brief    Abort device communication associated with a PDEV.
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_pdev_abort(uint64_t pdev_ptr)
{
    return val_smc_call(RMI_PDEV_ABORT, pdev_ptr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Get number of auxiliary Granules required for a PDEV.
 *   @param    value         -  PDEV flags value
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_pdev_aux_count(uint64_t value)
{
    return val_smc_call(RMI_PDEV_AUX_COUNT, value, 0, 0, 0, 0, 0, 0, 0, 0, 0);
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
val_smc_param_ts val_host_rmi_pdev_create_smc(uint64_t pdev_ptr, uint64_t params_ptr)
{
    return val_smc_call(RMI_PDEV_CREATE, pdev_ptr, params_ptr, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Destroy a PDEV
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_pdev_destroy_smc(uint64_t pdev_ptr)
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
 *   @brief    Refresh keys in an IDE connection between the Root Port and the endpoint device.
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @param    coh              -  Select coherent or non-coherent IDE stream
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_pdev_ide_key_refresh(uint64_t pdev_ptr, uint8_t coh)
{
    return val_smc_call(RMI_PDEV_IDE_KEY_REFRESH, pdev_ptr, coh, 0, 0, 0, 0, 0, 0, 0, 0);
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
 *   @brief    Provide public key associated with a PDEV.
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @param    params_ptr       -  PA of the key parameters
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_pdev_set_pubkey(uint64_t pdev_ptr, uint64_t params_ptr)
{
    return val_smc_call(RMI_PDEV_SET_PUBKEY, pdev_ptr, params_ptr, 0, 0, 0, 0, 0, 0, 0, 0);

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
 *   @param    rd               -  PA of the RD
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @param    vdev_ptr         -  PA of the VDEV
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_vdev_abort(uint64_t rd, uint64_t pdev_ptr, uint64_t vdev_ptr)
{
    return val_smc_call(RMI_VDEV_ABORT, rd, pdev_ptr, vdev_ptr, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Get number of auxiliary Granules required for a VDEV.
 *   @param    pdev_flags         -  PDEV flags
 *   @param    vdev_flags         -  VDEV flags
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_vdev_aux_count(uint64_t pdev_flags, uint64_t vdev_flags)
{
    return val_smc_call(RMI_VDEV_AUX_COUNT, pdev_flags, vdev_flags, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Perform device communication associated with a VDEV.
 *   @param    rd         -  PA of the RD
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @param    vdev_ptr         -  PA of the VDEV
 *   @param    data_ptr         -  PA of the communication data structure
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_vdev_communicate(uint64_t rd, uint64_t pdev_ptr,
                                         uint64_t vdev_ptr, uint64_t data_ptr)
{
    return val_smc_call(RMI_VDEV_COMMUNICATE, rd, pdev_ptr, vdev_ptr, data_ptr, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Create a VDEV.
 *   @param    rd               -  PA of the RD
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @param    vdev_ptr         -  PA of the VDEV
 *   @param    params_ptr       -  PA of VDEV parameters
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_vdev_create_smc(uint64_t rd, uint64_t pdev_ptr,
                                  uint64_t vdev_ptr, uint64_t params_ptr)
{
    return val_smc_call(RMI_VDEV_CREATE, rd, pdev_ptr, vdev_ptr, params_ptr, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Destroy a VDEV.
 *   @param    rd               -  PA of the RD
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @param    vdev_ptr         -  PA of the VDEV
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_vdev_destroy_smc(uint64_t rd, uint64_t pdev_ptr,
                                               uint64_t vdev_ptr)
{
    return val_smc_call(RMI_VDEV_DESTROY, rd, pdev_ptr, vdev_ptr, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Get VDEV interface report.
 *   @param    rd               -  PA of the RD
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @param    vdev_ptr         -  PA of the VDEV
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_vdev_get_interface_report(uint64_t rd, uint64_t pdev_ptr,
                                                                     uint64_t vdev_ptr)
{
    return val_smc_call(RMI_VDEV_GET_INTERFACE_REPORT, rd, pdev_ptr, vdev_ptr,
                                                         0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Get VDEV measurements.
 *   @param    rd               -  PA of the RD
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @param    vdev_ptr         -  PA of the VDEV
 *   @param    params_ptr       -  PA of VDEV parameters
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_vdev_get_measurements(uint64_t rd, uint64_t pdev_ptr,
                                            uint64_t vdev_ptr, uint64_t params_ptr)
{
    return val_smc_call(RMI_VDEV_GET_MEASUREMENTS, rd, pdev_ptr, vdev_ptr, params_ptr,
                                                                    0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Get state of a VDEV.
 *   @param    vdev_ptr         -  PA of the VDEV
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_vdev_get_state(uint64_t vdev_ptr)
{
    return val_smc_call(RMI_VDEV_GET_STATE, vdev_ptr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Lock VDEV.
 *   @param    rd               -  PA of the RD
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @param    vdev_ptr         -  PA of the VDEV
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_vdev_lock(uint64_t rd, uint64_t pdev_ptr, uint64_t vdev_ptr)
{
    return val_smc_call(RMI_VDEV_LOCK, rd, pdev_ptr, vdev_ptr, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Start VDEV.
 *   @param    rd               -  PA of the RD
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @param    vdev_ptr         -  PA of the VDEV
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_vdev_start(uint64_t rd, uint64_t pdev_ptr, uint64_t vdev_ptr)
{
    return val_smc_call(RMI_VDEV_START, rd, pdev_ptr, vdev_ptr, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Unlock a VDEV.
 *   @param    rd               -  PA of the RD
 *   @param    pdev_ptr         -  PA of the PDEV
 *   @param    vdev_ptr         -  PA of the VDEV
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_vdev_unlock(uint64_t rd, uint64_t pdev_ptr, uint64_t vdev_ptr)
{
    return val_smc_call(RMI_VDEV_UNLOCK, rd, pdev_ptr, vdev_ptr, 0, 0, 0, 0, 0, 0, 0);
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

/**
 *   @brief    Create initial mappings to protected data memory
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    data         -  PA of the target data
 *   @param    ipa          -  IPA at which the Granule will be mapped in the target Realm
 *   @param    src          -  PA of the source Granule
 *   @param    flags        -  Flags
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_data_map_init_smc(uint64_t rd, uint64_t data,
                   uint64_t ipa, uint64_t src, uint64_t flags)
{
    val_smc_param_ts args;

    args = val_smc_call(RMI_RTT_DATA_MAP_INIT, rd, data, ipa, src, flags, 0, 0, 0, 0, 0);
    if (args.x0)
        return args;

    val_host_update_granule_state(rd, GRANULE_DATA, data, ipa, 0, 0);
    return args;
}

/**
 *   @brief    Creates a mapping from a Protected IPA to a Data Granule
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    ipa          -  IPA at which the Granule will be mapped in the target Realm
 *   @param    level        -  RTT level
 *   @param    desc         -  RTTE descriptor
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_rtt_map_protected(uint64_t rd, uint64_t ipa,
                  uint64_t level, uint64_t desc)
{
    return (val_smc_call(RMI_RTT_MAP_PROTECTED, rd, ipa, level, desc, 0, 0, 0, 0, 0, 0)).x0;
}

/**
 *   @brief    Set RMM configuration
 *   @param    cfg_ptr      -  PA of RMM configuration structure
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rmm_config_set(uint64_t cfg_ptr)
{
    return val_smc_call(RMI_RMM_CONFIG_SET, cfg_ptr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Get configuration of a Granule tracking region
 *   @param    base         -  Base PA of the tracking region range
 *   @param    top          -  Top PA of the tracking region range
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_granule_tracking_get(uint64_t base, uint64_t top)
{
    return val_smc_call(RMI_GRANULE_TRACKING_GET, base, top,
                        0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Set configuration of a Granule tracking region
 *   @param    addr         -  PA of the tracking region
 *   @param    category     -  Memory category
 *   @param    state        -  Tracking region state
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_granule_tracking_set_smc(uint64_t addr,
                    uint64_t category, uint64_t state)
{
    return val_smc_call(RMI_GRANULE_TRACKING_SET, addr, category, state, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Get RMM configuration
 *   @param    cfg_ptr      -  PA of RMM configuration structure
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rmm_config_get(uint64_t cfg_ptr)
{
    return val_smc_call(RMI_RMM_CONFIG_GET, cfg_ptr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Get RMM state
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rmm_state_get(void)
{
    return val_smc_call(RMI_RMM_STATE_GET, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Delegates a range of Granules
 *   @param    base         -  Base PA of the target range
 *   @param    top          -  Top PA of the target range
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_granule_range_delegate_smc(uint64_t base, uint64_t top)
{
    return val_smc_call(RMI_GRANULE_RANGE_DELEGATE, base, top, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Undelegates a range of Granules
 *   @param    base         -  Base PA of the target range
 *   @param    top          -  Top PA of the target range
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_granule_range_undelegate_smc(uint64_t base, uint64_t top)
{
    return val_smc_call(RMI_GRANULE_RANGE_UNDELEGATE, base, top, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Create a Level 1 GPT
 *   @param    addr         -  Base of physical address region described by the L1GPT
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_gpt_l1_create_smc(uint64_t addr)
{
    return val_smc_call(RMI_GPT_L1_CREATE, addr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Destroy a Level 1 GPT
 *   @param    addr         -  Base of physical address region described by the L1GPT
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_gpt_l1_destroy_smc(uint64_t addr)
{
    return val_smc_call(RMI_GPT_L1_DESTROY, addr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Create mappings to conventional memory within a Protected IPA range
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    base         -  Base of the target IPA range
 *   @param    top          -  Top of the target IPA range
 *   @param    flags        -  Flags
 *   @param    oaddr        -  Output address set descriptor
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_data_map_smc(uint64_t rd, uint64_t base,
                    uint64_t top, uint64_t flags, uint64_t oaddr)
{
    return val_smc_call(RMI_RTT_DATA_MAP, rd, base, top, flags, oaddr, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Remove mappings to conventional memory within a Protected IPA range
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    base         -  Base of the target IPA range
 *   @param    top          -  Top of the target IPA range
 *   @param    flags        -  Flags
 *   @param    oaddr        -  Output address set descriptor
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_data_unmap_smc(uint64_t rd, uint64_t base,
                    uint64_t top, uint64_t flags, uint64_t oaddr)
{
    return val_smc_call(RMI_RTT_DATA_UNMAP, rd, base, top, flags, oaddr, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Create mappings to conventional memory within an Unprotected IPA range
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    base         -  Base of the target IPA range
 *   @param    top          -  Top of the target IPA range
 *   @param    flags        -  Flags
 *   @param    oaddr        -  Output address set descriptor
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_unprot_map_smc(uint64_t rd, uint64_t base,
                    uint64_t top, uint64_t flags, uint64_t oaddr)
{
    return val_smc_call(RMI_RTT_UNPROT_MAP, rd, base, top, flags, oaddr, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Remove mappings to conventional memory within an Unprotected IPA range
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    base         -  Base of the target IPA range
 *   @param    top          -  Top of the target IPA range
 *   @param    flags        -  Flags
 *   @param    oaddr        -  Output address set descriptor
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_unprot_unmap_smc(uint64_t rd, uint64_t base,
                    uint64_t top, uint64_t flags, uint64_t oaddr)
{
    return val_smc_call(RMI_RTT_UNPROT_UNMAP, rd, base, top, flags, oaddr, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Terminate a Realm
 *   @param    rd           -  PA of the RD
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_realm_terminate_smc(uint64_t rd)
{
    return val_smc_call(RMI_REALM_TERMINATE, rd, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Activate the RMM
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rmm_activate_smc(void)
{
    return val_smc_call(RMI_RMM_ACTIVATE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Continue a stateful RMI operation
 *   @param    handle       -  Handle which identifies the operation
 *   @param    flags        -  Flags
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_op_continue_smc(uint64_t handle, uint64_t flags)
{
    return val_smc_call(RMI_OP_CONTINUE, handle, flags, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Donate memory to a stateful RMI operation
 *   @param    handle       -  Handle which identifies the operation
 *   @param    list_addr    -  PA of the input Granule list
 *   @param    list_count   -  Number of entries in the input Granule list
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_op_mem_donate_smc(uint64_t handle, uint64_t list_addr,
                    uint64_t list_count)
{
    return val_smc_call(RMI_OP_MEM_DONATE, handle, list_addr, list_count, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Reclaim memory from a stateful RMI operation
 *   @param    handle       -  Handle which identifies the operation
 *   @param    list_addr    -  PA of the output Granule list
 *   @param    list_count   -  Number of entries in the output Granule list
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_op_mem_reclaim_smc(uint64_t handle, uint64_t list_addr,
                    uint64_t list_count)
{
    return val_smc_call(RMI_OP_MEM_RECLAIM, handle, list_addr, list_count, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Cancel a stateful RMI operation
 *   @param    handle       -  Handle which identifies the operation
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_op_cancel_smc(uint64_t handle)
{
    return val_smc_call(RMI_OP_CANCEL, handle, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Validate mappings to device memory from a target IPA range
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    rec_ptr      -  PA of the target REC
 *   @param    base         -  Base of the target IPA range
 *   @param    top          -  Top of the target IPA range
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_dev_validate_smc(uint64_t rd, uint64_t rec_ptr,
                                                   uint64_t base, uint64_t top)
{
    return val_smc_call(RMI_RTT_DEV_VALIDATE, rd, rec_ptr, base, top, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Refresh keys for a PDEV stream
 *   @param    pdev_1_ptr   -  PA of the first PDEV
 *   @param    pdev_2_ptr   -  PA of the second PDEV
 *   @param    stream_hnd   -  Stream handle
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_pdev_stream_key_refresh(uint64_t pdev_1_ptr,
                    uint64_t pdev_2_ptr, uint64_t stream_hnd)
{
    return val_smc_call(RMI_PDEV_STREAM_KEY_REFRESH, pdev_1_ptr, pdev_2_ptr, stream_hnd,
                                                                    0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Create mappings to device memory within a target Protected IPA range
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    vdev_ptr     -  PA of the VDEV
 *   @param    base         -  Base of the target IPA range
 *   @param    top          -  Top of the target IPA range
 *   @param    flags        -  Flags
 *   @param    oaddr        -  Output address set descriptor
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_dev_map_smc(uint64_t rd, uint64_t vdev_ptr,
                    uint64_t base, uint64_t top, uint64_t flags, uint64_t oaddr)
{
    return val_smc_call(RMI_RTT_DEV_MAP, rd, vdev_ptr, base, top, flags, oaddr, 0, 0, 0, 0);
}

/**
 *   @brief    Remove mappings to device memory within a target Protected IPA range
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    base         -  Base of the target IPA range
 *   @param    top          -  Top of the target IPA range
 *   @param    flags        -  Flags
 *   @param    oaddr        -  Output address set descriptor
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_rtt_dev_unmap_smc(uint64_t rd, uint64_t base,
                    uint64_t top, uint64_t flags, uint64_t oaddr)
{
    return val_smc_call(RMI_RTT_DEV_UNMAP, rd, base, top, flags, oaddr, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Connect a PDEV stream
 *   @param    params_ptr   -  PA of PDEV stream connect parameters
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_pdev_stream_connect(uint64_t params_ptr)
{
    return val_smc_call(RMI_PDEV_STREAM_CONNECT, params_ptr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Disconnect a PDEV stream
 *   @param    pdev_1_ptr   -  PA of the first PDEV
 *   @param    pdev_2_ptr   -  PA of the second PDEV
 *   @param    stream_hnd   -  Stream handle
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_pdev_stream_disconnect(uint64_t pdev_1_ptr,
                    uint64_t pdev_2_ptr, uint64_t stream_hnd)
{
    return val_smc_call(RMI_PDEV_STREAM_DISCONNECT, pdev_1_ptr, pdev_2_ptr, stream_hnd,
                                                                    0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Complete a PDEV stream operation
 *   @param    pdev_1_ptr   -  PA of the first PDEV
 *   @param    pdev_2_ptr   -  PA of the second PDEV
 *   @param    stream_hnd   -  Stream handle
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_pdev_stream_complete(uint64_t pdev_1_ptr,
                    uint64_t pdev_2_ptr, uint64_t stream_hnd)
{
    return val_smc_call(RMI_PDEV_STREAM_COMPLETE, pdev_1_ptr, pdev_2_ptr, stream_hnd,
                                                                    0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Purge keys for a PDEV stream
 *   @param    pdev_1_ptr   -  PA of the first PDEV
 *   @param    pdev_2_ptr   -  PA of the second PDEV
 *   @param    stream_hnd   -  Stream handle
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_pdev_stream_key_purge(uint64_t pdev_1_ptr,
                    uint64_t pdev_2_ptr, uint64_t stream_hnd)
{
    return val_smc_call(RMI_PDEV_STREAM_KEY_PURGE, pdev_1_ptr, pdev_2_ptr, stream_hnd,
                                                                    0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Notify RMM of triggered PSMMU interrupts
 *   @param    psmmu_ptr    -  PA of PSMMU
 *   @param    irqs         -  Interrupts
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_psmmu_irq_notify(uint64_t psmmu_ptr, uint64_t irqs)
{
    return val_smc_call(RMI_PSMMU_IRQ_NOTIFY, psmmu_ptr, irqs, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Activate a PSMMU
 *   @param    psmmu_ptr    -  PA of PSMMU
 *   @param    params_ptr   -  PA of PSMMU parameters
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_psmmu_activate_smc(uint64_t psmmu_ptr,
                                                 uint64_t params_ptr)
{
    return val_smc_call(RMI_PSMMU_ACTIVATE, psmmu_ptr, params_ptr, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Deactivate a PSMMU
 *   @param    psmmu_ptr    -  PA of PSMMU
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_psmmu_deactivate_smc(uint64_t psmmu_ptr)
{
    return val_smc_call(RMI_PSMMU_DEACTIVATE, psmmu_ptr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Create a PSMMU Level 2 Stream Table
 *   @param    psmmu_ptr    -  PA of PSMMU
 *   @param    sid          -  Stream ID
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_psmmu_st_l2_create_smc(uint64_t psmmu_ptr,
                                                     uint64_t sid)
{
    return val_smc_call(RMI_PSMMU_ST_L2_CREATE, psmmu_ptr, sid, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Destroy a PSMMU Level 2 Stream Table
 *   @param    psmmu_ptr    -  PA of PSMMU
 *   @param    sid          -  Stream ID
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_psmmu_st_l2_destroy_smc(uint64_t psmmu_ptr,
                                                      uint64_t sid)
{
    return val_smc_call(RMI_PSMMU_ST_L2_DESTROY, psmmu_ptr, sid, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Create a Level 0 DPT
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_dpt_l0_create(void)
{
    return val_smc_call(RMI_DPT_L0_CREATE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Destroy a Level 0 DPT
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_dpt_l0_destroy(void)
{
    return val_smc_call(RMI_DPT_L0_DESTROY, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Create a Level 1 DPT
 *   @param    addr         -  Base of physical address region described by the L1DPT
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_dpt_l1_create(uint64_t addr)
{
    return val_smc_call(RMI_DPT_L1_CREATE, addr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Destroy a Level 1 DPT
 *   @param    addr         -  Base of physical address region described by the L1DPT
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_dpt_l1_destroy(uint64_t addr)
{
    return val_smc_call(RMI_DPT_L1_DESTROY, addr, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Consume a PSMMU event
 *   @param    psmmu_ptr    -  PA of PSMMU
 *   @param    irq          -  IRQ
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_psmmu_event_consume(uint64_t psmmu_ptr, uint64_t irq)
{
    return val_smc_call(RMI_PSMMU_EVENT_CONSUME, psmmu_ptr, irq, 0, 0, 0, 0, 0, 0, 0, 0);
}

/**
 *   @brief    Get PSMMU information
 *   @param    psmmu_ptr    -  PA of PSMMU
 *   @param    info_ptr     -  PA of PSMMU information structure
 *   @return   SMC return arguments
**/
val_smc_param_ts val_host_rmi_psmmu_info(uint64_t psmmu_ptr, uint64_t info_ptr)
{
    return val_smc_call(RMI_PSMMU_INFO, psmmu_ptr, info_ptr, 0, 0, 0, 0, 0, 0, 0, 0);
}
