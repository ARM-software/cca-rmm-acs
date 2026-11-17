/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_sro.h"
#include "val_host_realm.h"
#include "val_host_rmi.h"
#include "val_rmm.h"

/**
 *   @brief    Activates RMM and completes SRO if operation is stateful
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_rmm_activate(void)
{
    val_smc_param_ts args;

    args = val_host_rmi_rmm_activate_smc();

    return val_host_sro_complete(&args);
}

/**
 *   @brief    Delegates a granule range and completes SRO if operation is stateful
 *   @param    base         -  Base PA of target granule range
 *   @param    top          -  Top PA of target granule range
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_granule_range_delegate(uint64_t base, uint64_t top)
{
    val_smc_param_ts args;
    uint64_t ret;
    uint64_t next = base, prev;
    uint32_t busy_count = 0;

    do
    {
        prev = next;
        args = val_host_rmi_granule_range_delegate_smc(next, top);

        if (RMI_STATUS(args.x0) == RMI_BUSY)
        {
            if (++busy_count >= VAL_HOST_SRO_MAX_ITERATIONS)
            {
                LOG(ERROR, "GRANULE_RANGE_DELEGATE busy retry limit reached\n");
                return VAL_ERROR;
            }
            continue;
        }

        busy_count = 0;
        ret = val_host_sro_complete(&args);
        if (RMI_STATUS(ret) != RMI_SUCCESS)
            return ret;

        if ((args.x1 <= next) || (args.x1 > top))
            return VAL_ERROR;

        while (prev < args.x1)
        {
            val_host_add_granule(GRANULE_DELEGATED, prev, NULL);
            prev += PAGE_SIZE;
        }

        next = args.x1;
    } while (next < top);

    return RMI_SUCCESS;
}

/**
 *   @brief    Undelegates a granule range and completes SRO if operation is stateful
 *   @param    base         -  Base PA of target granule range
 *   @param    top          -  Top PA of target granule range
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_granule_range_undelegate(uint64_t base, uint64_t top)
{
    val_smc_param_ts args;
    uint64_t ret;
    uint64_t next = base, prev;
    uint32_t busy_count = 0;

    do
    {
        prev = next;
        args = val_host_rmi_granule_range_undelegate_smc(next, top);

        if (RMI_STATUS(args.x0) == RMI_BUSY)
        {
            if (++busy_count >= VAL_HOST_SRO_MAX_ITERATIONS)
            {
                LOG(ERROR, "GRANULE_RANGE_UNDELEGATE busy retry limit reached\n");
                return VAL_ERROR;
            }
            continue;
        }

        busy_count = 0;
        ret = val_host_sro_complete(&args);
        if (RMI_STATUS(ret) != RMI_SUCCESS)
            return ret;

        if ((args.x1 <= next) || (args.x1 > top))
            return VAL_ERROR;

        while (prev < args.x1)
        {
            val_host_update_destroy_granule_state(0, prev, 0, 0, GRANULE_UNDELEGATED, 0, 0);
            prev += PAGE_SIZE;
        }

        next = args.x1;
    } while (next < top);

    return RMI_SUCCESS;
}

/**
 *   @brief    Configures a Granule tracking region and completes SRO
 *   @param    addr         -  PA of the tracking region
 *   @param    category     -  Memory category
 *   @param    state        -  Tracking region state
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_granule_tracking_set(uint64_t addr,
                                           uint64_t category,
                                           uint64_t state)
{
    val_smc_param_ts args;

    args = val_host_rmi_granule_tracking_set_smc(addr, category, state);

    return val_host_sro_complete(&args);
}

/**
 *   @brief    Creates a Level 1 GPT and completes SRO if operation is stateful
 *   @param    addr         - Base of physical address region described by the L1GPT
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_gpt_l1_create(uint64_t addr)
{
    val_smc_param_ts args;

    args = val_host_rmi_gpt_l1_create_smc(addr);

    return val_host_sro_complete(&args);
}

/**
 *   @brief    Destroys a Level 1 GPT and completes SRO if operation is stateful
 *   @param    addr         - Base of physical address region described by the L1GPT
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_gpt_l1_destroy(uint64_t addr)
{
    val_smc_param_ts args;

    args = val_host_rmi_gpt_l1_destroy_smc(addr);

    return val_host_sro_complete(&args);
}

/**
 *   @brief    Creates a PDEV and completes SRO if operation is stateful
 *   @param    pdev_ptr     -  PA of the PDEV
 *   @param    params_ptr   -  PA of PDEV parameters
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_pdev_create(uint64_t pdev_ptr, uint64_t params_ptr)
{
    val_smc_param_ts args;

    args = val_host_rmi_pdev_create_smc(pdev_ptr, params_ptr);

    return val_host_sro_complete(&args);
}

/**
 *   @brief    Destroys a PDEV and completes SRO if operation is stateful
 *   @param    pdev_ptr     -  PA of the PDEV
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_pdev_destroy(uint64_t pdev_ptr)
{
    val_smc_param_ts args;

    args = val_host_rmi_pdev_destroy_smc(pdev_ptr);

    return val_host_sro_complete(&args);
}

/**
 *   @brief    Activates a PSMMU and completes SRO if operation is stateful
 *   @param    psmmu_ptr    -  PA of the PSMMU
 *   @param    params_ptr   -  PA of PSMMU parameters
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_psmmu_activate(uint64_t psmmu_ptr, uint64_t params_ptr)
{
    val_smc_param_ts args;

    args = val_host_rmi_psmmu_activate_smc(psmmu_ptr, params_ptr);

    return val_host_sro_complete(&args);
}

/**
 *   @brief    Deactivates a PSMMU and completes SRO if operation is stateful
 *   @param    psmmu_ptr    -  PA of the PSMMU
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_psmmu_deactivate(uint64_t psmmu_ptr)
{
    val_smc_param_ts args;

    args = val_host_rmi_psmmu_deactivate_smc(psmmu_ptr);

    return val_host_sro_complete(&args);
}

/**
 *   @brief    Creates a PSMMU L2 stream table and completes SRO if operation is stateful
 *   @param    psmmu_ptr    -  PA of the PSMMU
 *   @param    sid          -  Stream ID covered by the L2 table
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_psmmu_st_l2_create(uint64_t psmmu_ptr, uint64_t sid)
{
    val_smc_param_ts args;

    args = val_host_rmi_psmmu_st_l2_create_smc(psmmu_ptr, sid);

    return val_host_sro_complete(&args);
}

/**
 *   @brief    Destroys a PSMMU L2 stream table and completes SRO if operation is stateful
 *   @param    psmmu_ptr    -  PA of the PSMMU
 *   @param    sid          -  Stream ID covered by the L2 table
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_psmmu_st_l2_destroy(uint64_t psmmu_ptr, uint64_t sid)
{
    val_smc_param_ts args;

    args = val_host_rmi_psmmu_st_l2_destroy_smc(psmmu_ptr, sid);

    return val_host_sro_complete(&args);
}

/**
 *   @brief    Creates a VDEV and completes SRO if operation is stateful
 *   @param    rd           -  PA of the RD
 *   @param    pdev_ptr     -  PA of the PDEV
 *   @param    vdev_ptr     -  PA of the VDEV
 *   @param    params_ptr   -  PA of VDEV parameters
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_vdev_create(uint64_t rd, uint64_t pdev_ptr,
                                  uint64_t vdev_ptr, uint64_t params_ptr)
{
    val_smc_param_ts args;

    args = val_host_rmi_vdev_create_smc(rd, pdev_ptr, vdev_ptr, params_ptr);

    return val_host_sro_complete(&args);
}

/**
 *   @brief    Destroys a VDEV and completes SRO if operation is stateful
 *   @param    rd           -  PA of the RD
 *   @param    pdev_ptr     -  PA of the PDEV
 *   @param    vdev_ptr     -  PA of the VDEV
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_vdev_destroy(uint64_t rd, uint64_t pdev_ptr,
                                   uint64_t vdev_ptr)
{
    val_smc_param_ts args;

    args = val_host_rmi_vdev_destroy_smc(rd, pdev_ptr, vdev_ptr);

    return val_host_sro_complete(&args);
}

/**
 *   @brief    Creates a Realm and completes SRO if operation is stateful
 *   @param    rd           -  PA of the Realm Descriptor
 *   @param    params_ptr   -  PA of Realm parameters
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_realm_create(uint64_t rd, uint64_t params_ptr)
{
    val_smc_param_ts args;
    uint64_t ret;
    uint64_t initial_status;

    args = val_host_rmi_realm_create_smc(rd, params_ptr);
    initial_status = RMI_STATUS(args.x0);
    ret = val_host_sro_complete(&args);

    if (ret != RMI_SUCCESS)
        return ret;

    if (initial_status != RMI_SUCCESS)
        val_host_update_granule_state(rd, GRANULE_RD, rd, 0, 0, 0);

    return ret;
}

/**
 *   @brief    Destroys a Realm and completes SRO if operation is stateful
 *   @param    rd           -  PA of the Realm Descriptor
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_realm_destroy(uint64_t rd)
{
    val_smc_param_ts args;
    uint64_t ret;
    uint64_t initial_status;

    args = val_host_rmi_realm_destroy_smc(rd);
    initial_status = RMI_STATUS(args.x0);
    ret = val_host_sro_complete(&args);

    if (ret != RMI_SUCCESS)
        return ret;

    if (initial_status != RMI_SUCCESS)
        val_host_update_destroy_granule_state(rd, rd, 0, 0, GRANULE_DELEGATED, GRANULE_RD, 0);

    return ret;
}
/**
 *   @brief    Creates a REC and completes SRO if operation is stateful
 *   @param    rd           -  PA of the Realm Descriptor
 *   @param    rec          -  PA of REC granule
 *   @param    params_ptr   -  PA of REC parameters
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_rec_create(uint64_t rd, uint64_t rec, uint64_t params_ptr)
{
    val_smc_param_ts args;
    uint64_t ret;

    args = val_host_rmi_rec_create_smc(rd, rec, params_ptr);
    ret = val_host_sro_complete(&args);

    if (ret != RMI_SUCCESS)
        return ret;

    val_host_update_granule_state(rd, GRANULE_REC, rec, 0, 0, 0);

    return ret;
}

/**
 *   @brief    Destroys a REC and completes SRO if operation is stateful
 *   @param    rec          -  PA of REC granule
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_rec_destroy(uint64_t rec)
{
    val_smc_param_ts args;
    uint64_t ret;
    uint64_t initial_status;

    args = val_host_rmi_rec_destroy_smc(rec);
    initial_status = RMI_STATUS(args.x0);
    ret = val_host_sro_complete(&args);

    if (ret != RMI_SUCCESS)
        return ret;

    if (initial_status != RMI_SUCCESS)
        val_host_update_destroy_granule_state(0, rec, 0, 0,
                                              GRANULE_DELEGATED, GRANULE_REC, 0);

    return ret;
}

/**
 *   @brief    Creates an RTT and completes SRO if operation is stateful
 *   @param    rd           -  PA of the Realm Descriptor
 *   @param    rtt          -  PA of RTT granule
 *   @param    ipa          -  Base IPA mapped by RTT
 *   @param    level        -  RTT level
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_rtt_create(uint64_t rd, uint64_t rtt, uint64_t ipa,
                                 uint64_t level)
{
    val_smc_param_ts args;
    uint64_t ret;
    uint64_t initial_status;

    args = val_host_rmi_rtt_create_smc(rd, rtt, ipa, level);
    initial_status = RMI_STATUS(args.x0);
    ret = val_host_sro_complete(&args);

    if (ret != RMI_SUCCESS)
        return ret;

    if (initial_status != RMI_SUCCESS)
        val_host_update_granule_state(rd, GRANULE_RTT, rtt, ipa, level, 0);

    return ret;
}

/**
 *   @brief    Folds an RTT and completes SRO if operation is stateful
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    ipa          -  Base IPA mapped by RTT
 *   @param    level        -  RTT level
 *   @param    rtt          -  Pointer to store PA of the destroyed RTT
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_rtt_fold(uint64_t rd, uint64_t ipa, uint64_t level,
                               uint64_t *rtt)
{
    val_smc_param_ts args;
    uint64_t ret;
    uint64_t initial_status;

    args = val_host_rmi_rtt_fold_smc(rd, ipa, level);
    initial_status = RMI_STATUS(args.x0);
    ret = val_host_sro_complete(&args);
    if (ret != RMI_SUCCESS)
        return ret;

    *rtt = args.x1;
    if (initial_status != RMI_SUCCESS)
        val_host_update_destroy_granule_state(rd, 0, ipa, level,
                                              GRANULE_DELEGATED, GRANULE_RTT, 0);

    return ret;
}

/**
 *   @brief    Destroys an RTT and completes SRO if operation is stateful
 *   @param    rd           -  PA of the RD for the target Realm
 *   @param    ipa          -  Base IPA mapped by RTT
 *   @param    level        -  RTT level
 *   @param    rtt_destroy  -  Pointer to store output values for given ABI
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_rtt_destroy(uint64_t rd, uint64_t ipa, uint64_t level,
                                  val_host_rtt_destroy_ts *rtt_destroy)
{
    val_smc_param_ts args;
    uint64_t ret;
    uint64_t initial_status;

    args = val_host_rmi_rtt_destroy_smc(rd, ipa, level);
    initial_status = RMI_STATUS(args.x0);
    ret = val_host_sro_complete(&args);

    rtt_destroy->rtt = args.x1;
    rtt_destroy->top = args.x2;

    if (ret != RMI_SUCCESS)
        return ret;

    if (initial_status != RMI_SUCCESS)
        val_host_update_destroy_granule_state(rd, 0, ipa, level,
                                              GRANULE_DELEGATED, GRANULE_RTT, 0);

    return ret;
}

/**
 *   @brief    Creates initial protected data mapping and completes SRO if operation is stateful
 *   @param    rd           -  PA of the Realm Descriptor
 *   @param    data         -  PA of Data granule
 *   @param    ipa          -  IPA of Data granule
 *   @param    src          -  PA of source granule
 *   @param    flags        -  Mapping flags
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_rtt_data_map_init(uint64_t rd, uint64_t data,
                                        uint64_t ipa, uint64_t src,
                                        uint64_t flags)
{
    val_smc_param_ts args;
    uint64_t ret;
    uint64_t initial_status;

    args = val_host_rmi_rtt_data_map_init_smc(rd, data, ipa, src, flags);
    initial_status = RMI_STATUS(args.x0);
    ret = val_host_sro_complete(&args);

    if (ret != RMI_SUCCESS)
        return ret;

    if (initial_status != RMI_SUCCESS)
        val_host_update_granule_state(rd, GRANULE_DATA, data, ipa, 0, 0);

    return ret;
}

/**
 *   @brief    Creates protected data mappings and completes SRO if operation is stateful
 *   @param    rd           -  PA of the Realm Descriptor
 *   @param    base         -  Base IPA of mapping range
 *   @param    top          -  Top IPA of mapping range
 *   @param    flags        -  Mapping flags
 *   @param    oaddr        -  Output address descriptor
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_rtt_data_map(uint64_t rd, uint64_t base,
                                   uint64_t top, uint64_t flags,
                                   uint64_t oaddr)
{
    val_smc_param_ts args;
    val_host_rmi_rtt_prot_map_flags_ts map_flags;
    uint64_t ret;

    args = val_host_rmi_rtt_data_map_smc(rd, base, top, flags, oaddr);
    ret = val_host_sro_complete(&args);

    if (ret != RMI_SUCCESS)
        return ret;

    val_memset(&map_flags, 0x0, sizeof(map_flags));
    val_memcpy(&map_flags, &flags, sizeof(map_flags));
    if (map_flags.oaddr_type == RMI_ADDR_TYPE_SINGLE)
        val_host_update_granule_state(rd, GRANULE_DATA,
                                      ADDR_ALIGN_DOWN(oaddr, PAGE_SIZE),
                                      base, 0, 0);

    return ret;
}

/**
 *   @brief    Removes protected data mappings and completes SRO if operation is stateful
 *   @param    rd           -  PA of the Realm Descriptor
 *   @param    base         -  Base IPA of mapping range
 *   @param    top          -  Top IPA of mapping range
 *   @param    flags        -  Unmap flags
 *   @param    oaddr        -  Output address descriptor
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_rtt_data_unmap(uint64_t rd, uint64_t base,
                                     uint64_t top, uint64_t flags,
                                     uint64_t oaddr)
{
    val_smc_param_ts args;
    val_host_rmi_rtt_unmap_flags_ts unmap_flags;
    uint64_t ret;

    args = val_host_rmi_rtt_data_unmap_smc(rd, base, top, flags, oaddr);
    ret = val_host_sro_complete(&args);

    if (ret != RMI_SUCCESS)
        return ret;

    val_memset(&unmap_flags, 0x0, sizeof(unmap_flags));
    val_memcpy(&unmap_flags, &flags, sizeof(unmap_flags));
    if (unmap_flags.oaddr_type == RMI_ADDR_TYPE_SINGLE)
        val_host_update_destroy_granule_state(rd,
                                           ADDR_ALIGN_DOWN(args.x2, PAGE_SIZE),
                                           base, 0,
                                           GRANULE_DELEGATED, GRANULE_DATA, 0);

    return ret;
}

/**
 *   @brief    Creates device mappings and completes SRO if operation is stateful
 *   @param    rd           -  PA of the Realm Descriptor
 *   @param    vdev_ptr     -  PA of the VDEV
 *   @param    base         -  Base IPA of mapping range
 *   @param    top          -  Top IPA of mapping range
 *   @param    flags        -  Mapping flags
 *   @param    oaddr        -  Output address descriptor
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_rtt_dev_map(uint64_t rd, uint64_t vdev_ptr,
                                  uint64_t base, uint64_t top,
                                  uint64_t flags, uint64_t oaddr)
{
    val_smc_param_ts args;

    args = val_host_rmi_rtt_dev_map_smc(rd, vdev_ptr, base, top, flags, oaddr);

    return val_host_sro_complete(&args);
}

/**
 *   @brief    Removes device mappings and completes SRO if operation is stateful
 *   @param    rd           -  PA of the Realm Descriptor
 *   @param    base         -  Base IPA of mapping range
 *   @param    top          -  Top IPA of mapping range
 *   @param    flags        -  Unmap flags
 *   @param    oaddr        -  Output address descriptor
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_rtt_dev_unmap(uint64_t rd, uint64_t base,
                                    uint64_t top, uint64_t flags,
                                    uint64_t oaddr)
{
    val_smc_param_ts args;

    args = val_host_rmi_rtt_dev_unmap_smc(rd, base, top, flags, oaddr);

    return val_host_sro_complete(&args);
}

/**
 *   @brief    Validates device mappings and completes SRO if operation is stateful
 *   @param    rd           -  PA of the Realm Descriptor
 *   @param    rec_ptr      -  PA of the target REC
 *   @param    base         -  Base IPA of validation range
 *   @param    top          -  Top IPA of validation range
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_rtt_dev_validate(uint64_t rd, uint64_t rec_ptr,
                                       uint64_t base, uint64_t top,
                                       uint64_t *out_top)
{
    val_smc_param_ts args;
    uint64_t ret;

    args = val_host_rmi_rtt_dev_validate_smc(rd, rec_ptr, base, top);
    ret = val_host_sro_complete(&args);
    if ((ret == RMI_SUCCESS) && (out_top != NULL))
        *out_top = args.x1;

    return ret;
}

/**
 *   @brief    Initializes RIPAS and completes SRO if operation is stateful
 *   @param    rd           -  PA of the Realm Descriptor
 *   @param    base         -  Base IPA of target range
 *   @param    top          -  Top IPA of target range
 *   @param    out_top      -  Pointer to output top IPA progressed by RMM
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_rtt_init_ripas(uint64_t rd, uint64_t base, uint64_t top,
                                     uint64_t *out_top)
{
    val_smc_param_ts args;
    uint64_t ret;

    args = val_host_rmi_rtt_init_ripas_smc(rd, base, top, out_top);
    ret = val_host_sro_complete(&args);
    *out_top = args.x1;

    return ret;
}

/**
 *   @brief    Sets RIPAS and completes SRO if operation is stateful
 *   @param    rd           -  PA of the Realm Descriptor
 *   @param    rec          -  PA of REC granule
 *   @param    base         -  Base IPA of target range
 *   @param    top          -  Top IPA of target range
 *   @param    out_top      -  Pointer to output top IPA progressed by RMM
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_rtt_set_ripas(uint64_t rd, uint64_t rec, uint64_t base,
                                    uint64_t top, uint64_t *out_top)
{
    val_smc_param_ts args;
    uint64_t ret;

    args = val_host_rmi_rtt_set_ripas_smc(rd, rec, base, top, out_top);
    ret = val_host_sro_complete(&args);
    *out_top = args.x1;

    return ret;
}

/**
 *   @brief    Creates unprotected mappings and completes SRO if operation is stateful
 *   @param    rd           -  PA of the Realm Descriptor
 *   @param    base         -  Base IPA of mapping range
 *   @param    top          -  Top IPA of mapping range
 *   @param    flags        -  Mapping flags
 *   @param    oaddr        -  Output address descriptor
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_rtt_unprot_map(uint64_t rd, uint64_t base,
                                     uint64_t top, uint64_t flags, uint64_t oaddr)
{
    val_smc_param_ts args;
    val_host_rmi_rtt_unprot_map_flags_ts map_flags;
    uint64_t ret;

    args = val_host_rmi_rtt_unprot_map_smc(rd, base, top, flags, oaddr);
    ret = val_host_sro_complete(&args);

    if (ret != RMI_SUCCESS)
        return ret;

    val_memset(&map_flags, 0x0, sizeof(map_flags));
    val_memcpy(&map_flags, &flags, sizeof(map_flags));
    if (map_flags.oaddr_type == RMI_ADDR_TYPE_SINGLE)
        val_host_update_granule_state(rd, GRANULE_UNPROTECTED, base, base,
                                      VAL_RTT_MAX_LEVEL -
                                      VAL_EXTRACT_BITS(oaddr, 0, 1), 0);

    return ret;
}

/**
 *   @brief    Removes unprotected mappings and completes SRO if operation is stateful
 *   @param    rd           -  PA of the Realm Descriptor
 *   @param    base         -  Base IPA of mapping range
 *   @param    top          -  Top IPA of mapping range
 *   @param    flags        -  Unmap flags
 *   @param    oaddr        -  Output address descriptor
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_rtt_unprot_unmap(uint64_t rd, uint64_t base,
                                       uint64_t top, uint64_t flags, uint64_t oaddr)
{
    val_smc_param_ts args;
    val_host_rmi_rtt_unmap_flags_ts unmap_flags;
    uint64_t ret;

    args = val_host_rmi_rtt_unprot_unmap_smc(rd, base, top, flags, oaddr);
    ret = val_host_sro_complete(&args);

    if (ret != RMI_SUCCESS)
        return ret;

    val_memset(&unmap_flags, 0x0, sizeof(unmap_flags));
    val_memcpy(&unmap_flags, &flags, sizeof(unmap_flags));
    if (unmap_flags.oaddr_type == RMI_ADDR_TYPE_SINGLE)
        val_host_update_destroy_granule_state(rd, base, 0, 0,
                                              GRANULE_UNPROTECTED,
                                              GRANULE_UNPROTECTED, 0);

    return ret;
}

/**
 *   @brief    Terminates a Realm and completes SRO if operation is stateful
 *   @param    rd           -  PA of the Realm Descriptor
 *   @return   Final RMI status
**/
uint64_t val_host_rmi_realm_terminate(uint64_t rd)
{
    val_smc_param_ts args;

    args = val_host_rmi_realm_terminate_smc(rd);
    return val_host_sro_complete(&args);
}
