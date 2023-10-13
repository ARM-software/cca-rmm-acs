/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_rmi.h"
#include "val_libc.h"
#include "val_host_realm.h"

/**
 *   @brief    Returns RMI version
 *   @param    void
 *   @return   Returns RMI Interface version
**/
uint64_t val_host_rmi_version(void)
{
    return (val_smc_call(RMI_VERSION, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)).x0;
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
    val_host_update_granule_state(rd, GRANULE_DATA, data, ipa, 0);
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
    val_host_update_granule_state(rd, GRANULE_DATA, data, ipa, 0);
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
    val_host_update_granule_state(0, GRANULE_DELEGATED, addr, 0, 0);
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
    val_host_update_destroy_granule_state(0, addr, 0, 0, GRANULE_UNDELEGATED, 0);
    return ret;
}

/**
 *   @brief    Completes a pending PSCI command which was called with an MPIDR argument,
 *             by providing the corresponding REC
 *   @param    calling_rec         -  PA of the calling REC
 *   @param    target_rec          -  PA of the target REC
 *   @return   Returns command return status
**/
uint64_t val_host_rmi_psci_complete(uint64_t calling_rec,
                 uint64_t target_rec)
{
    return (val_smc_call(RMI_PSCI_COMPLETE, calling_rec, target_rec, 0, 0, 0, 0, 0, 0, 0, 0)).x0;
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
    val_host_update_granule_state(rd, GRANULE_RD, rd, 0, 0);
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
    val_host_update_destroy_granule_state(rd, rd, 0, 0, GRANULE_DELEGATED, GRANULE_RD);
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
    val_host_update_destroy_granule_state(rd, 0, ipa, 0, GRANULE_DELEGATED, GRANULE_DATA);
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
    val_host_update_granule_state(rd, GRANULE_REC, rec, 0, 0);
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
    val_host_update_destroy_granule_state(0, rec, 0, 0, GRANULE_DELEGATED, GRANULE_REC);
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
    val_host_rec_entry_flags_ts rec_entry_flags;
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
        rec_entry_flags.emul_mmio = 0;
        rec_entry_flags.inject_sea = 0;
        val_memcpy(&run->entry.flags, &rec_entry_flags, sizeof(rec_entry_flags));
        val_host_realm_printf_msg_service();
        goto rec_enter;
    }

    return ret;
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
    val_host_update_granule_state(rd, GRANULE_RTT, rtt, ipa, level);
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

    val_host_update_destroy_granule_state(rd, 0, ipa, level, GRANULE_DELEGATED, GRANULE_RTT);
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

    val_host_update_destroy_granule_state(rd, 0, ipa, level, GRANULE_DELEGATED, GRANULE_RTT);
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
    val_host_update_granule_state(rd, GRANULE_UNPROTECTED, ipa, ipa, level);
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
    val_host_update_destroy_granule_state(rd, ipa, 0, 0, GRANULE_UNPROTECTED, GRANULE_UNPROTECTED);
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
