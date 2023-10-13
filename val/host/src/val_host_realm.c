/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_realm.h"
#include "val_host_alloc.h"
#include "val_host_helpers.h"

int current_realm = 1;
val_host_granule_ts *head = NULL;
val_host_granule_ts *current = NULL;
val_host_granule_ts *tail = NULL;

val_host_memory_track_ts mem_track[VAL_HOST_MAX_REALMS] = {
    {.rd = 0x00000000FFFFFFFF},
    {.rd = 0x00000000FFFFFFFF},
    {.rd = 0x00000000FFFFFFFF},
    {.rd = 0x00000000FFFFFFFF},
    {.rd = 0x00000000FFFFFFFF},
    {.rd = 0x00000000FFFFFFFF},
    {.rd = 0x00000000FFFFFFFF},
    {.rd = 0x00000000FFFFFFFF},
    {.rd = 0x00000000FFFFFFFF},
    {.rd = 0x00000000FFFFFFFF}
};

static uint64_t val_host_rtt_level_mapsize(uint64_t rtt_level)
{
    if (rtt_level > VAL_RTT_MAX_LEVEL)
        return PAGE_SIZE;

    return (1UL << VAL_RTT_LEVEL_SHIFT(rtt_level));
}

static uint64_t val_host_rtt_create(uint64_t phys,
                val_host_realm_ts *realm,
                uint64_t rtt_addr,
                uint64_t rtt_level
                )
{
    rtt_addr = ADDR_ALIGN_DOWN(rtt_addr, val_host_rtt_level_mapsize(rtt_level - 1));
    if (rtt_level == 3)
    {
        realm->rtt_l3[realm->rtt_l3_count].rtt_addr = phys;
        realm->rtt_l3[realm->rtt_l3_count].ipa = rtt_addr;
        realm->rtt_l3_count++;
    } else if (rtt_level == 2)
    {
        realm->rtt_l2[realm->rtt_l2_count].rtt_addr = phys;
        realm->rtt_l2[realm->rtt_l2_count].ipa = rtt_addr;
        realm->rtt_l2_count++;
    } else if (rtt_level == 1)
    {
        realm->rtt_l1[realm->rtt_l1_count].rtt_addr = phys;
        realm->rtt_l1[realm->rtt_l1_count].ipa = rtt_addr;
        realm->rtt_l1_count++;
    }
    return val_host_rmi_rtt_create(realm->rd, phys, rtt_addr, rtt_level);
}

/**
 *   @brief    Create RTT levels
 *   @param    realm         - Realm strucrure
 *   @param    ipa           - IPA Address
 *   @param    rtt_level     - Starting RTT level
 *   @param    rtt_max_level - Maximum RTT level
 *   @param    rtt_alignment - RTT Address Alignment
 *   @return   SUCCESS/FAILURE
**/
uint32_t val_host_create_rtt_levels(val_host_realm_ts *realm,
                   uint64_t ipa,
                   uint64_t rtt_level,
                   uint64_t rtt_max_level,
                   uint64_t rtt_alignment)
{
    uint64_t rtt;

    if (rtt_level > rtt_max_level)
        return VAL_ERROR;

    for (; rtt_level++ < rtt_max_level;)
    {
        rtt = (uint64_t)val_host_mem_alloc(rtt_alignment, PAGE_SIZE);
        if (!rtt)
        {
            LOG(ERROR, "\tFailed to allocate memory for rtt\n", 0, 0);
            return VAL_ERROR;
        } else if (val_host_rmi_granule_delegate(rtt))
        {
            LOG(ERROR, "\tRtt delegation failed, rtt=0x%x\n", rtt, 0);
            return VAL_ERROR;
        }

        if (val_host_rtt_create(rtt, realm, ipa, rtt_level))
        {
            LOG(ERROR, "\tRtt create failed, rtt=0x%x\n", rtt, 0);
            val_host_rmi_granule_undelegate(rtt);
            val_host_mem_free((void *)rtt);
            return VAL_ERROR;
        }
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    Maps protected memory into the realm
 *   @param    realm        - Realm strucrure
 *   @param    target_pa    - PA of target data
 *   @param    ipa          - IPA Address
 *   @param    rtt_map_size - size of memory to be mapped
 *   @param    src_pa       - PA of source granule
 *   @return   SUCCESS/FAILURE
**/
uint32_t val_host_map_protected_data(val_host_realm_ts *realm,
                uint64_t target_pa,
                uint64_t ipa,
                uint64_t rtt_map_size,
                uint64_t src_pa)
{
    uint64_t rd = realm->rd;
    uint64_t map_level, rtt_level;
    uint64_t ret = 0;
    uint64_t size = 0;
    uint64_t phys = target_pa;
    uint64_t flags = RMI_NO_MEASURE_CONTENT;
    val_host_rtt_entry_ts  rtte;
    val_host_data_destroy_ts data_destroy;

    if (!ADDR_IS_ALIGNED(ipa, rtt_map_size))
        return VAL_ERROR;

    if (rtt_map_size == PAGE_SIZE)
        map_level = 3;
    else if (rtt_map_size == VAL_RTT_L2_BLOCK_SIZE)
        map_level = 2;
    else
        {
            LOG(ERROR, "\tUnknown rtt_map_size=0x%x\n", rtt_map_size, 0);
            return VAL_ERROR;
        }

    while (size < rtt_map_size)
    {
        if (val_host_rmi_granule_delegate(phys))
        {
            LOG(ERROR, "\tGranule delegation failed, PA=0x%x\n", phys, 0);
            return VAL_ERROR;
        }

        ret = val_host_rmi_data_create(rd, phys, ipa, src_pa, flags);

        if (RMI_STATUS(ret) == RMI_ERROR_RTT)
        {
            rtt_level = RMI_INDEX(ret);
            ret = val_host_rmi_rtt_read_entry(realm->rd,
                        val_host_addr_align_to_level(ipa, rtt_level), rtt_level, &rtte);
            if (ret)
            {
                LOG(ERROR, "\tval_host_rmi_rtt_read_entry, ret=0x%x\n", ret, 0);
                return VAL_ERROR;
            }

            if (rtte.state == RMI_UNASSIGNED)
            {
                /* Create missing RTT levels and retry data create again */
                ret = val_host_create_rtt_levels(realm, ipa, (uint32_t)rtte.walk_level,
                                map_level, PAGE_SIZE);
                if (ret)
                {
                    LOG(ERROR, "\tval_realm_create_rtt_levels failed, ret=0x%x\n", ret, 0);
                    goto error;
                }

                ret = val_host_rmi_data_create(rd, phys, ipa, src_pa, flags);
            }
        }

        if (ret)
        {
            LOG(ERROR, "\tval_rmi_data_create failed, ret=0x%x\n", ret, 0);
            goto error;
        }

        phys += PAGE_SIZE;
        src_pa += PAGE_SIZE;
        ipa += PAGE_SIZE;
        size += PAGE_SIZE;
    }

    return VAL_SUCCESS;

error:
    for (; size > 0;)
    {
        ret = val_host_rmi_data_destroy(rd, ipa, &data_destroy);
        if (ret)
            LOG(ERROR, "\tval_rmi_rtt_mapprotected failed, ret=0x%x\n", ret, 0);

        ret = val_host_rmi_granule_undelegate(phys);
        if (ret)
        {
            LOG(ERROR, "\tval_rmi_granule_undelegate failed\n", 0, 0);
        }
        phys -= PAGE_SIZE;
        size -= PAGE_SIZE;
        ipa -= PAGE_SIZE;
    }

    return VAL_ERROR;
}

/**
 *   @brief    Maps protected memory into the realm with unknown contents
 *   @param    realm        - Realm strucrure
 *   @param    target_pa    - PA of target data
 *   @param    ipa          - IPA Address
 *   @param    rtt_map_size - size of memory to be mapped
 *   @return   SUCCESS/FAILURE
**/
uint32_t val_host_map_protected_data_unknown(val_host_realm_ts *realm,
                        uint64_t target_pa,
                        uint64_t ipa,
                        uint64_t rtt_map_size)
{
    uint64_t rd = realm->rd;
    uint32_t map_level, rtt_level;
    uint64_t ret = 0;
    uint64_t size = 0;
    uint64_t phys = target_pa;
    val_host_rtt_entry_ts rtte;
    val_host_data_destroy_ts data_destroy;

    if (!ADDR_IS_ALIGNED(ipa, rtt_map_size))
        return VAL_ERROR;

    if (rtt_map_size == PAGE_SIZE)
        map_level = 3;
    else if (rtt_map_size == VAL_RTT_L2_BLOCK_SIZE)
        map_level = 2;
    else
        {
            LOG(ERROR, "\tUnknown rtt_map_size=0x%x\n", rtt_map_size, 0);
            return VAL_ERROR;
        }

    while (size < rtt_map_size)
    {
        if (val_host_rmi_granule_delegate(phys))
        {
            LOG(ERROR, "\tGranule delegation failed, PA=0x%x\n", phys, 0);
            return VAL_ERROR;
        }

        ret = val_host_rmi_data_create_unknown(rd, phys, ipa);

        if (RMI_STATUS(ret) == RMI_ERROR_RTT)
        {
            rtt_level = RMI_INDEX(ret);
            ret = val_host_rmi_rtt_read_entry(realm->rd,
                            val_host_addr_align_to_level(ipa, rtt_level), rtt_level, &rtte);
            if (ret)
            {
                LOG(ERROR, "\tval_host_rmi_rtt_read_entry, ret=0x%x\n", ret, 0);
                return VAL_ERROR;
            }

            if (rtte.state == RMI_UNASSIGNED)
            {
                /* Create missing RTT levels and retry data create unknown again */
                ret = val_host_create_rtt_levels(realm, ipa, (uint32_t)rtte.walk_level,
                                map_level, PAGE_SIZE);
                if (ret)
                {
                    LOG(ERROR, "\tval_realm_create_rtt_levels failed, ret=0x%x\n", ret, 0);
                    goto error;
                }

                ret = val_host_rmi_data_create_unknown(rd, phys, ipa);
            }
        }

        if (ret)
        {
            LOG(ERROR, "\tval_rmi_data_create_unknown failed, ret=0x%x\n", ret, 0);
            goto error;
        }

        phys += PAGE_SIZE;
        ipa += PAGE_SIZE;
        size += PAGE_SIZE;
    }

    return VAL_SUCCESS;

error:
    for (; size > 0;)
    {
        val_host_rmi_data_destroy(rd, ipa, &data_destroy);
        if (ret)
            LOG(ERROR, "\tval_rmi_rtt_mapprotected failed, ret=0x%x\n", ret, 0);

        ret = val_host_rmi_granule_undelegate(phys);
        if (ret)
        {
            LOG(ERROR, "\tval_rmi_granule_undelegate failed\n", 0, 0);
        }
        phys -= PAGE_SIZE;
        size -= PAGE_SIZE;
        ipa -= PAGE_SIZE;
    }
    return VAL_ERROR;

}

/**
 *   @brief    Creates a mapping from an Unprotected IPA to a Non-secure PA
 *   @param    realm            - Realm strucrure
 *   @param    ns_pa            - Non secure Physical Address
 *   @param    ipa              - IPA Address
 *   @param    rtt_map_size     - size of memory to be mapped
 *   @param    rtt_alignment    - RTT Address Alignment
 *   @return   SUCCESS/FAILURE
**/
uint32_t val_host_map_unprotected(val_host_realm_ts *realm,
                        uint64_t ns_pa,
                        uint64_t ipa,
                        uint64_t rtt_map_size,
                        uint64_t rtt_alignment)
{
    uint64_t rd = realm->rd;
    uint64_t map_level, rtt_level;
    uint64_t ret = 0;
    uint64_t mem_desc = ns_pa | ATTR_NORMAL_WB | ATTR_STAGE2_MASK | ATTR_INNER_SHARED;
    val_host_rtt_entry_ts rtte;

    if (!ADDR_IS_ALIGNED(ipa, rtt_map_size))
        return VAL_ERROR;

    if (rtt_map_size == PAGE_SIZE)
        map_level = 3;
    else if (rtt_map_size == VAL_RTT_L2_BLOCK_SIZE)
        map_level = 2;
    else
        {
            LOG(ERROR, "\tUnknown rtt_map_size=0x%x\n", rtt_map_size, 0);
            return VAL_ERROR;
        }

    ret = val_host_rmi_rtt_map_unprotected(rd, ipa, map_level, mem_desc);

    if (RMI_STATUS(ret) == RMI_ERROR_RTT)
        {
            rtt_level = RMI_INDEX(ret);
            ret = val_host_rmi_rtt_read_entry(realm->rd,
                       val_host_addr_align_to_level(ipa, rtt_level), rtt_level, &rtte);

            if (ret)
            {
                LOG(ERROR, "\tval_host_rmi_rtt_read_entry, ret=0x%x\n", ret, 0);
                return VAL_ERROR;
            }

            if (rtte.state == RMI_UNASSIGNED)
            {
                /* Create missing RTT levels and retry map unprotected again */
                ret = val_host_create_rtt_levels(realm, ipa, rtte.walk_level,
                                                   map_level, rtt_alignment);
                if (ret)
                {
                    LOG(ERROR, "\tval_realm_create_rtt_levels failed, ret=0x%x\n", ret, 0);
                    return VAL_ERROR;
                }

                ret = val_host_rmi_rtt_map_unprotected(rd, ipa, map_level, mem_desc);
            }
    }

    if (ret)
    {
        LOG(ERROR, "\tval_rmi_rtt_map_unprotected failed, ret=0x%x\n", ret, 0);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    Creates a mapping from an Unprotected IPA to a Non-secure PA
 *   @param    realm            - Realm strucrure
 *   @param    ns_pa            - Non secure Physical Address
 *   @param    ipa              - IPA Address
 *   @param    rtt_map_size     - size of memory to be mapped
 *   @param    rtt_alignment    - RTT Address Alignment
 *   @param    mem_attr         - Memory attributes
 *   @return   SUCCESS/FAILURE
**/
uint32_t val_host_map_unprotected_attr(val_host_realm_ts *realm,
                        uint64_t ns_pa,
                        uint64_t ipa,
                        uint64_t rtt_map_size,
                        uint64_t rtt_alignment, uint64_t mem_attr)
{
    uint64_t rd = realm->rd;
    uint64_t map_level, rtt_level;
    uint64_t ret = 0;
    uint64_t mem_desc = 0;
    val_host_rtt_entry_ts rtte;

    mem_desc = ns_pa | mem_attr;

    if (!ADDR_IS_ALIGNED(ipa, rtt_map_size))
        return VAL_ERROR;

    if (rtt_map_size == PAGE_SIZE)
        map_level = 3;
    else if (rtt_map_size == VAL_RTT_L2_BLOCK_SIZE)
        map_level = 2;
    else
        {
            LOG(ERROR, "\tUnknown rtt_map_size=0x%x\n", rtt_map_size, 0);
            return VAL_ERROR;
        }

    ret = val_host_rmi_rtt_map_unprotected(rd, ipa, map_level, mem_desc);

    if (RMI_STATUS(ret) == RMI_ERROR_RTT)
        {
            rtt_level = RMI_INDEX(ret);
            ret = val_host_rmi_rtt_read_entry(realm->rd,
                        val_host_addr_align_to_level(ipa, rtt_level), rtt_level, &rtte);
            if (ret)
            {
                LOG(ERROR, "\tval_host_rmi_rtt_read_entry, ret=0x%x\n", ret, 0);
                return VAL_ERROR;
            }

            if (rtte.state == RMI_UNASSIGNED)
            {
                /* Create missing RTT levels and retry map unprotected again */
                ret = val_host_create_rtt_levels(realm, ipa, rtt_level, map_level, rtt_alignment);
                if (ret)
                {
                    LOG(ERROR, "\tval_realm_create_rtt_levels failed, ret=0x%x\n", ret, 0);
                return VAL_ERROR;
                }

                ret = val_host_rmi_rtt_map_unprotected(rd, ipa, map_level, mem_desc);
            }
    }

    if (ret)
    {
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    Creates realm
 *   @param    realm            - Realm strucrure
 *   @return   SUCCESS/FAILURE
**/
uint32_t val_host_realm_create(val_host_realm_ts *realm)
{
    val_host_realm_params_ts *params;
    uint64_t ret, i;

    realm->image_pa_size = PLATFORM_REALM_IMAGE_SIZE;

    realm->state = REALM_STATE_NULL;

    /* Allocate memory for realm image. Granule delegation
     * for it will be performed during rtt creation.  */
    realm->image_pa_base = (uint64_t)val_host_mem_alloc(PAGE_SIZE, realm->image_pa_size);
    if (!realm->image_pa_base)
    {
        LOG(ERROR, "\tval_host_mem_alloc failed, base=0x%x, size=0x%x\n",
                realm->image_pa_base, realm->image_pa_size);
        return VAL_ERROR;
    }


    /* Allocate and delegate RTT */
    realm->rtt_l0_addr = (uint64_t)val_host_mem_alloc((realm->num_s2_sl_rtts * PAGE_SIZE),
                                                    (realm->num_s2_sl_rtts * PAGE_SIZE));

    if (!realm->rtt_l0_addr)
    {
        LOG(ERROR, "\tFailed to allocate memory for rtt_addr\n", 0, 0);
        goto undelegate_rd;
    } else {
        for (i = 0; i < realm->num_s2_sl_rtts; i++)
        {
            ret = val_host_rmi_granule_delegate(realm->rtt_l0_addr + (i * PAGE_SIZE));
            if (ret)
            {
                LOG(ERROR, "\trtt delegation failed, rtt_addr=0x%x, ret=0x%x\n",
                    realm->rtt_l0_addr, ret);
                goto free_rtt;
            }
        }
    }

    /* Allocate and delegate RD */
    realm->rd = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!realm->rd)
    {
        LOG(ERROR, "\tFailed to allocate memory for rd\n", 0, 0);
        goto free_par;
    } else {
        ret = val_host_rmi_granule_delegate(realm->rd);
        if (ret)
        {
            LOG(ERROR, "\trd delegation failed, rd=0x%x, ret=0x%x\n", realm->rd, ret);
            goto free_rd;
        }
    }

    /* Allocate memory for params */
    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    if (params == NULL)
    {
        LOG(ERROR, "\tFailed to allocate memory for params\n", 0, 0);
        goto undelegate_rtt;
    }
    val_memset(params, 0, PAGE_SIZE_4K);

    /* Populate params */
    params->flags = realm->flags;
    params->pmu_num_ctrs = realm->pmu_num_ctrs;
    params->s2sz = realm->s2sz;
    params->rtt_base = realm->rtt_l0_addr;
    params->hash_algo = realm->hash_algo;
    params->rtt_level_start = realm->s2_starting_level;
    params->rtt_num_start = realm->num_s2_sl_rtts;
    params->vmid = realm->vmid;
    val_memcpy(&params->rpv, &realm->rpv, sizeof(realm->rpv));

    /* Create realm */
    if (val_host_rmi_realm_create(realm->rd, (uint64_t)params))
    {
        LOG(ERROR, "\tRealm create failed\n", 0, 0);
        goto free_params;
    }

    realm->state = REALM_STATE_NEW;

    /* Free params */
    val_host_mem_free(params);
    return VAL_SUCCESS;

free_params:
    val_host_mem_free(params);

undelegate_rtt:
    ret = val_host_rmi_granule_undelegate(realm->rtt_l0_addr);
    if (ret)
    {
        LOG(WARN, "\trtt undelegation failed, rtt_addr=0x%x, ret=0x%x\n", realm->rtt_l0_addr, ret);
    }

free_rtt:
     val_host_mem_free((void *)realm->rtt_l0_addr);

undelegate_rd:
    ret = val_host_rmi_granule_undelegate(realm->rd);
    if (ret)
    {
        LOG(WARN, "\trd undelegation failed, rd=0x%x, ret=0x%x\n", realm->rd, ret);
    }
free_rd:
     val_host_mem_free((void *)realm->rd);

free_par:
     val_host_mem_free((void *)realm->image_pa_base);

    return VAL_ERROR;
}

/**
 *   @brief    Set the RIPAS of a target IPA range to RAM
 *   @param    realm            - Realm strucrure
 *   @param    base             - Base of target IPA region
 *   @param    top              - Top of target IPA region
 *   @param    rtt_level        - RTT level
 *   @param    rtt_alignment    - RTT Address Alignment
 *   @return   SUCCESS/FAILURE
**/
int val_host_ripas_init(val_host_realm_ts *realm, uint64_t base,
                uint64_t top, uint64_t rtt_level, uint64_t rtt_alignment)
{
    uint64_t ret = 0, out_top, rtt_level1;
    val_host_rtt_entry_ts rtte;

    do
    {
        ret = val_host_rmi_rtt_init_ripas(realm->rd, base, top, &out_top);
        rtt_level1 = RMI_INDEX(ret);

        if (RMI_STATUS(ret) == RMI_ERROR_RTT && rtt_level1 < VAL_RTT_MAX_LEVEL)
        {
            ret = val_host_rmi_rtt_read_entry(realm->rd,
                        val_host_addr_align_to_level(base, rtt_level1), rtt_level1, &rtte);
            if (ret)
            {
                LOG(ERROR, "\tval_host_rmi_rtt_read_entry, ret=0x%x\n", ret, 0);
                return VAL_ERROR;
            }

            if (rtte.state == RMI_UNASSIGNED)
            {
                ret = val_host_create_rtt_levels(realm, base, rtte.walk_level,
                                                rtt_level, rtt_alignment);
                if (ret)
                {
                    return VAL_ERROR;
                }
                continue;
            }
            return VAL_ERROR;
        }
        else if (ret)
        {
            return VAL_ERROR;
        }
    } while (out_top != top);

    return VAL_SUCCESS;
}

/**
 *   @brief    Creates memory mappings for realm image
 *   @param    realm            - Realm strucrure
 *   @return   SUCCESS/FAILURE
**/
static uint32_t val_host_image_map(val_host_realm_ts *realm)
{
    uint64_t src_pa = PLATFORM_REALM_IMAGE_BASE;
    uint32_t i = 0;

    if (val_host_ripas_init(realm,
            VAL_REALM_IMAGE_BASE_IPA,
            VAL_REALM_IMAGE_BASE_IPA + realm->image_pa_size,
            VAL_RTT_MAX_LEVEL, PAGE_SIZE))
    {
        LOG(ERROR, "\trealm_init_ipa_state failed, ipa=0x%x\n",
                realm->image_pa_base + i * PAGE_SIZE, 0);
        return VAL_ERROR;
    }
    /* MAP image regions */
    while (i < (realm->image_pa_size/PAGE_SIZE))
    {
        if (val_host_map_protected_data(realm,
                realm->image_pa_base + i * PAGE_SIZE,
                VAL_REALM_IMAGE_BASE_IPA + i * PAGE_SIZE,
                PAGE_SIZE,
                src_pa + i * PAGE_SIZE
                ))
        {
            LOG(ERROR, "\tval_realm_map_protected_data failed, par_base=0x%x\n",
                    realm->image_pa_base, 0);
            return VAL_ERROR;
        }
        i++;
    }
    realm->granules[realm->granules_mapped_count].ipa = VAL_REALM_IMAGE_BASE_IPA;
    realm->granules[realm->granules_mapped_count].size = realm->image_pa_size;
    realm->granules[realm->granules_mapped_count].level = VAL_RTT_MAX_LEVEL;
    realm->granules[realm->granules_mapped_count].pa = realm->image_pa_base;
    realm->granules_mapped_count++;
    return VAL_SUCCESS;
}
/**
 *   @brief    Maps protected memory into the realm
 *   @param    realm            - Realm strucrure
 *   @param    data_create      - Data creation structure
 *   @return   SUCCESS/FAILURE
**/
uint32_t val_host_map_protected_data_to_realm(val_host_realm_ts *realm,
                                            val_data_create_ts *data_create)
{
    uint32_t i = 0;

    if (val_host_ripas_init(realm,
            data_create->ipa,
            data_create->ipa + data_create->size,
            VAL_RTT_MAX_LEVEL, data_create->rtt_alignment))
    {
        LOG(ERROR, "\tval_host_ripas_init failed, ipa=0x%x\n",
                data_create->ipa + i * PAGE_SIZE, 0);
        return VAL_ERROR;
    }
    /* MAP image regions */
    while (i < (data_create->size/PAGE_SIZE))
    {
        if (val_host_map_protected_data(realm,
                data_create->target_pa + i * PAGE_SIZE,
                data_create->ipa + i * PAGE_SIZE,
                PAGE_SIZE,
                data_create->src_pa + i * PAGE_SIZE
                ))
        {
            LOG(ERROR, "\tval_realm_map_protected_data failed, par_base=0x%x\n",
                    data_create->target_pa, 0);
            return VAL_ERROR;
        }
        i++;
    }

    realm->granules[realm->granules_mapped_count].ipa = data_create->ipa;
    realm->granules[realm->granules_mapped_count].size = data_create->size;
    realm->granules[realm->granules_mapped_count].level = VAL_RTT_MAX_LEVEL;
    realm->granules[realm->granules_mapped_count].pa = data_create->target_pa;
    realm->granules_mapped_count++;
    return VAL_SUCCESS;
}

/**
 *   @brief    Creates memory mappings for shared region
 *   @param    realm            - Realm strucrure
 *   @return   SUCCESS/FAILURE
**/
static uint32_t val_host_map_shared_region(val_host_realm_ts *realm)
{
    uint32_t i = 0;
    uint64_t ns_shared_base_pa = (uint64_t)val_get_shared_region_base_pa();
    uint64_t ns_shared_base_ipa =
                            (uint64_t)val_get_shared_region_base_ipa(realm->s2sz & 0xff);

    /* MAP SHARED_NS region */
    while (i < PLATFORM_SHARED_REGION_SIZE/PAGE_SIZE)
    {
        if (val_host_map_unprotected(realm,
                ns_shared_base_pa + i * PAGE_SIZE,
                ns_shared_base_ipa + i * PAGE_SIZE,
                PAGE_SIZE, PAGE_SIZE
                ))
        {
            LOG(ERROR, "\tval_realm_map_unprotected_data failed\n", 0, 0);
            return VAL_ERROR;
        }
        i++;
    }
    realm->granules[realm->granules_mapped_count].ipa = ns_shared_base_ipa;
    realm->granules[realm->granules_mapped_count].size = PLATFORM_SHARED_REGION_SIZE;
    realm->granules[realm->granules_mapped_count].level = VAL_RTT_MAX_LEVEL;
    realm->granules[realm->granules_mapped_count].pa = ns_shared_base_pa;
    realm->granules_mapped_count++;
    return VAL_SUCCESS;
}

/**
 *   @brief    Creates memory mappings for shared region for given size
 *   @param    realm      - Realm strucrure
 *   @param    size       - Size of memory
 *   @param    mem_attr   - Memory attributes
 *   @return   Success: returns index, Failure: returns 0
**/
uint32_t val_host_map_ns_shared_region(val_host_realm_ts *realm, uint64_t size, uint64_t mem_attr)
{
    uint32_t i = 0;
    uint64_t pa = 0;
    uint64_t ns_shared_base_ipa = 0;

    /* Allocate the NS memory */
    pa = (uint64_t)val_host_mem_alloc(PAGE_SIZE, size);
    if (!pa)
    {
        LOG(ERROR, "\tval_host_mem_alloc failed\n", 0, 0);
        return 0;
    }

    ns_shared_base_ipa =
        (uint64_t)val_get_ns_shared_region_base_ipa(realm->s2sz & 0xff, pa);
    /* MAP SHARED_NS region */
    while (i < size/PAGE_SIZE)
    {
        if (val_host_map_unprotected_attr(realm,
                pa + i * PAGE_SIZE,
                ns_shared_base_ipa + i * PAGE_SIZE,
                PAGE_SIZE, PAGE_SIZE,
                mem_attr
                ))
        {
            return 0;
        }
        i++;
    }
    realm->granules[realm->granules_mapped_count].ipa = ns_shared_base_ipa;
    realm->granules[realm->granules_mapped_count].size = size;
    realm->granules[realm->granules_mapped_count].level = VAL_RTT_MAX_LEVEL;
    realm->granules[realm->granules_mapped_count].pa = pa;
    realm->granules_mapped_count++;
    return (uint32_t)(realm->granules_mapped_count - 1);
}

/**
 *   @brief    Creates the REC
 *   @param    realm      - Realm strucrure
 *   @return   SUCCESS/FAILURE
**/
uint32_t val_host_rec_create(val_host_realm_ts *realm)
{
    val_host_rec_params_ts   *rec_params;
    val_host_rec_create_flags_ts rec_create_flags;

    uint64_t ret, i, mpidr = 0x0, aux_count, j;

    if (realm->rec_count > VAL_MAX_REC_COUNT)
    {
        LOG(ERROR, "\tmax supported recs are VAL_MAX_REC_COUNT\n", 0, 0);
        return VAL_ERROR;
    }

    /* Get aux granules count */
    ret = val_host_rmi_rec_aux_count(realm->rd, &aux_count);
    if (ret)
    {
        LOG(ERROR, "\tREC AUX count failed, ret=0x%x\n", ret, 0);
        return VAL_ERROR;
    } else {
        if (aux_count > VAL_MAX_REC_AUX_GRANULES)
        {
            LOG(ERROR, "\tmax supported aux granules are VAL_MAX_REC_AUX_GRANULES\n", 0, 0);
            return VAL_ERROR;
        }

    }

    /* Allocate memory for rec_params */
    rec_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (rec_params == NULL)
    {
        LOG(ERROR, "\tFailed to allocate memory for rec_params\n", 0, 0);
        return VAL_ERROR;
    }
    val_memset(rec_params, 0x0, PAGE_SIZE);
    val_memset(&rec_create_flags, 0, sizeof(rec_create_flags));

    /* Populate rec_params */
    rec_params->num_aux = aux_count;
    realm->aux_count = aux_count;

    for (i = 0; i < (sizeof(rec_params->gprs)/sizeof(rec_params->gprs[0])); i++)
    {
        rec_params->gprs[i] = 0x0;
    }

    rec_params->pc = VAL_REALM_IMAGE_BASE_IPA;
    rec_create_flags.runnable = RMI_RUNNABLE;
    val_memcpy(&rec_params->flags, &rec_create_flags, sizeof(rec_create_flags));

    for (i = 0; i < realm->rec_count; i++, mpidr++)
    {
        rec_params->mpidr = mpidr;
        /* Allocate memory for run object */
        realm->run[i] = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
        if (!realm->run[i])
        {
            LOG(ERROR, "\tFailed to allocate memory for run[%d]\n", i, 0);
            goto free_rec_params;
        }
        val_memset((void *)realm->run[i], 0x0, PAGE_SIZE);

        /* Allocate and delegate REC */
        realm->rec[i] = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
        if (!realm->rec[i])
        {
            LOG(ERROR, "\tFailed to allocate memory for REC\n", 0, 0);
            goto free_rec_params;
        } else {
            ret = val_host_rmi_granule_delegate(realm->rec[i]);
            if (ret)
            {
                LOG(ERROR, "\trec delegation failed, rec=0x%x, ret=0x%x\n", realm->rec[i], ret);
                goto free_rec_params;
            }
        }

        for (j = 0; j < aux_count; j++)
        {
            rec_params->aux[j] = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
            if (!rec_params->aux[j])
            {
                LOG(ERROR, "\tFailed to allocate memory for aux rec\n", 0, 0);
                goto free_rec_params;
            } else {
                ret = val_host_rmi_granule_delegate(rec_params->aux[j]);
                if (ret)
                {
                    LOG(ERROR, "\trec delegation failed, rec=0x%x, ret=0x%x\n",
                                         rec_params->aux[j], ret);
                    goto free_rec_params;
                }
            }
            realm->rec_aux_granules[j + (i * aux_count)] = rec_params->aux[j];
        }

        /* Create REC  */
        ret = val_host_rmi_rec_create(realm->rd, realm->rec[i], (uint64_t)rec_params);
        if (ret)
        {
            LOG(ERROR, "\tREC create failed, ret=0x%x\n", ret, 0);
            goto free_rec_params;
        }
    }

    /* Free rec_params */
    val_host_mem_free(rec_params);
    return VAL_SUCCESS;

free_rec_params:
    while (i > 0)
    {
        ret = val_host_rmi_granule_undelegate(realm->rec[i]);
        if (ret)
        {
            LOG(WARN, "\trec undelegation failed, rec=0x%x, ret=0x%x\n", realm->rec[i], ret);
        }

        val_host_mem_free((void *)realm->rec[i]);
        val_host_mem_free((void *)realm->run[i]);

        for (j = 0; j < aux_count; j++)
        {
            ret = val_host_rmi_granule_undelegate(realm->rec_aux_granules[j + (i * aux_count)]);
            if (ret)
            {
                LOG(WARN, "\tgranule undelegation failed, PA=0x%x, ret=0x%x\n",
                realm->rec_aux_granules[j + (i * aux_count)], ret);
            }
            val_host_mem_free((void *)realm->rec_aux_granules[j + (i * aux_count)]);
        }
        i--;
        if (i == 0)
        {
            break;
        }
    }

    val_host_mem_free(rec_params);
    return VAL_ERROR;
}

/**
 *   @brief    Activates the Realm
 *   @param    realm      - Realm strucrure
 *   @return   SUCCESS/FAILURE
**/
uint32_t val_host_realm_activate(val_host_realm_ts *realm)
{
    uint64_t ret;

    /* Activate Realm  */
    ret = val_host_rmi_realm_activate(realm->rd);
    if (ret)
    {
        LOG(ERROR, "\tRealm activate failed, ret=0x%x\n", ret, 0);
        return VAL_ERROR;
    }

    realm->state = REALM_STATE_ACTIVE;

    return VAL_SUCCESS;
}

/**
 *   @brief    Setting up realm
 *   @param    realm      - Realm strucrure
 *   @param    activate   - Boolean value for actiate realm
 *   @return   SUCCESS/FAILURE
**/
uint32_t val_host_realm_setup(val_host_realm_ts *realm, bool activate)
{
    /* Create realm */
    if (val_host_realm_create(realm))
    {
        LOG(ERROR, "\tRealm create failed\n", 0, 0);
        return VAL_ERROR;
    }

    /* Create RECs */
    if (val_host_rec_create(realm))
    {
        LOG(ERROR, "\tREC create failed\n", 0, 0);
        return VAL_ERROR;
    }

    /* RTT map realm image */
    if (val_host_image_map(realm))
    {
        LOG(ERROR, "\tRealm image mapping failed\n", 0, 0);
        return VAL_ERROR;
    }

     /* RTT map shared_ns */
    if (val_host_map_shared_region(realm))
    {
        LOG(ERROR, "\tShared region mapping failed\n", 0, 0);
        return VAL_ERROR;
    }
    if (activate == 1)
    {
        /* Activate realm */
        if (val_host_realm_activate(realm))
        {
            LOG(ERROR, "\tRealm activate failed\n", 0, 0);
            return VAL_ERROR;
        }
    }
   return VAL_SUCCESS;
}

/**
 *   @brief    Set the default realm params
 *   @param    realm      - Realm structure
 *   @return   void
**/
void val_host_realm_params(val_host_realm_ts *realm)
{
    realm->s2sz = IPA_WIDTH_DEFAULT;
    realm->hash_algo = RMI_HASH_SHA_256;
    realm->s2_starting_level = 1;
    realm->num_s2_sl_rtts = 1;
    realm->vmid = 0;
    realm->rec_count = 1;
}

/**
 *   @brief    Checks the realm exit state is ripas change
 *   @param    run      - Rec run structure pointer
 *   @return   SUCCESS/FAILURE
**/
uint32_t val_host_check_realm_exit_ripas_change(val_host_rec_run_ts *run)
{
    if (run->exit.exit_reason == RMI_EXIT_RIPAS_CHANGE)
        return VAL_SUCCESS;

    return VAL_ERROR;
}

/**
 *   @brief    Checks the realm exit state is host call
 *   @param    run      - Rec run structure pointer
 *   @return   SUCCESS/FAILURE
**/
uint32_t val_host_check_realm_exit_host_call(val_host_rec_run_ts *run)
{
    if ((run->exit.exit_reason == RMI_EXIT_HOST_CALL) &&
        (run->exit.imm == VAL_SWITCH_TO_HOST))
        return VAL_SUCCESS;

    return VAL_ERROR;
}

/**
 *   @brief    Checks the realm exit state is psci
 *   @param    run      - Rec run structure pointer
 *   @param    psci_fid - PSCI function id
 *   @return   SUCCESS/FAILURE
**/
uint32_t val_host_check_realm_exit_psci(val_host_rec_run_ts *run, uint32_t psci_fid)
{
    if ((run->exit.exit_reason == RMI_EXIT_PSCI) &&
        (run->exit.gprs[0] == psci_fid))
        return VAL_SUCCESS;

    return VAL_ERROR;
}

/**
 *   @brief    Add granule to the NS mem track[0]
 *   @param    state      - state of granule
 *   @param    PA         - Physical address of granule
 *   @param    node       - node pointer
 *   @return   void
**/
void val_host_add_granule(uint32_t state, uint64_t PA, val_host_granule_ts *node)
{
    val_host_granule_ts *granule_list;

    /* if node is null, create node and add to NS mem_track[0] list
       else add node directly to the list */
    if (node == NULL)
    {
        granule_list = (val_host_granule_ts *) mem_alloc(sizeof(val_host_granule_ts),
                                                              sizeof(val_host_granule_ts));
        granule_list->state = state;
        granule_list->PA = PA;
        granule_list->next = NULL;
        granule_list->is_granule_sliced = 0;
    } else
    {
        granule_list = node;
    }

    if (head == NULL)
    {
        head = granule_list;
        mem_track[0].gran_type.ns = head;
        mem_track[0].gran_type.ns->next = NULL;
        tail = granule_list;
    } else
    {
        tail->next = granule_list;
        tail = tail->next;
    }
}

/**
 *   @brief    Get the current realm from mem track
 *   @param    rd      -  Realm RD granule address
 *   @return   Returns current realm index from mem track structure/0
**/
int val_host_get_curr_realm(uint64_t rd)
{
    int i;

    for (i = 1; i < VAL_HOST_MAX_REALMS; i++)
    {
        if (mem_track[i].rd == rd)
        {
            return i;
        }
    }
    return 0;
}

/**
 *   @brief    Update the granule state in mem track
 *   @param    rd         - Realm rd
 *   @param    state      - state of granule
 *   @param    PA         - Physical address of granule
 *   @param    ipa        - IPA of granule
 *   @param    rtt_level  - RTT level
 *   @return   void
**/
void val_host_update_granule_state(uint64_t rd, uint32_t state, uint64_t PA,
                                               uint64_t ipa, uint64_t rtt_level)
{
    val_host_granule_ts *granule_node = NULL;
    int i;

    /* Get the current realm index for given realm rd */
    if (state != GRANULE_DELEGATED)
    {
        current_realm = val_host_get_curr_realm(rd);
    }

    /* find node from NS mem_track[0] */
    granule_node = val_host_find_granule(PA);

    /* if node is not found add to the NS/VALID_NS list */
    if (granule_node == NULL)
    {
        val_host_granule_ts *granule_list_delegated =
                             (val_host_granule_ts *) mem_alloc(sizeof(val_host_granule_ts),
                                                                    sizeof(val_host_granule_ts));
        granule_list_delegated->rd = rd;
        granule_list_delegated->state = state;
        granule_list_delegated->PA = PA;
        granule_list_delegated->ipa = ipa;
        granule_list_delegated->level = rtt_level;
        granule_list_delegated->next = NULL;

        if (state == GRANULE_DELEGATED)
        {
            granule_list_delegated->is_granule_sliced = 1;
            tail->next = granule_list_delegated;
            tail = tail->next;
        }

        if (state == GRANULE_UNPROTECTED)
        {
            current = mem_track[current_realm].gran_type.valid_ns;
            if (current == NULL)
            {
                 mem_track[current_realm].gran_type.valid_ns = granule_list_delegated;
            } else
            {
                while (current->next != NULL)
                {
                    current = current->next;
                }
                current->next = granule_list_delegated;
            }
        }
        return;
    }

    /* Update state from NS mem_track or remove node and add to the respective state list */
    current = head;

    switch (state)
    {
        case GRANULE_DELEGATED:
            granule_node->state = state;
            break;

        case GRANULE_RD:
            val_host_remove_granule(&mem_track[0].gran_type.ns, PA);
            granule_node->rd = rd;
            granule_node->state = state;
            granule_node->ipa = ipa;
            granule_node->level = rtt_level;
            granule_node->next = NULL;

            /* Add realm rd to the mem_track */
            for (i = 1; i < VAL_HOST_MAX_REALMS; i++)
            {
                if (mem_track[i].rd == PA)
                {
                    LOG(ERROR, "\tRealm already exists\n", 0, 0);
                    break;
                } else if (mem_track[i].rd == 0x00000000FFFFFFFF) {
                    current_realm = i;
                    mem_track[current_realm].rd = PA;
                    break;
                }
            }

            mem_track[current_realm].gran_type.rd = granule_node;
            break;

        case GRANULE_REC:
            val_host_remove_granule(&mem_track[0].gran_type.ns, PA);
            granule_node->rd = rd;
            granule_node->state = state;
            granule_node->ipa = ipa;
            granule_node->level = rtt_level;
            granule_node->next = NULL;
            current = mem_track[current_realm].gran_type.rec;

            if (current == NULL)
            {
                 mem_track[current_realm].gran_type.rec = granule_node;
            } else {
                while (current->next != NULL)
                {
                    current = current->next;
                }
                current->next = granule_node;
            }

            break;

        case GRANULE_RTT:
            val_host_remove_granule(&mem_track[0].gran_type.ns, PA);
            granule_node->rd = rd;
            granule_node->state = state;
            granule_node->ipa = ipa;
            granule_node->level = rtt_level;
            granule_node->next = NULL;

            current = mem_track[current_realm].gran_type.rtt;
            if (current == NULL)
            {
                 mem_track[current_realm].gran_type.rtt = granule_node;
            } else {
                while (current->next != NULL)
                {
                    current = current->next;
                }
                current->next = granule_node;
            }

            break;

        case GRANULE_DATA:
            val_host_remove_granule(&mem_track[0].gran_type.ns, PA);
            granule_node->rd = rd;
            granule_node->state = state;
            granule_node->ipa = ipa;
            granule_node->level = rtt_level;
            granule_node->next = NULL;

            current = mem_track[current_realm].gran_type.data;
            if (current == NULL)
            {
                 mem_track[current_realm].gran_type.data = granule_node;
            } else {
                while (current->next != NULL)
                {
                    current = current->next;
                }
                current->next = granule_node;
            }

            break;

        case GRANULE_UNPROTECTED:
            val_host_remove_granule(&mem_track[0].gran_type.ns, PA);
            granule_node->rd = rd;
            granule_node->ipa = ipa;
            granule_node->level = rtt_level;
            granule_node->next = NULL;

            current = mem_track[current_realm].gran_type.valid_ns;
            if (current == NULL)
            {
                 mem_track[current_realm].gran_type.valid_ns = granule_node;
            } else {
                while (current->next != NULL)
                {
                    current = current->next;
                }
                current->next = granule_node;
            }

            break;

        default:
            break;
    }
}

/**
 *   @brief    Return the granule from NS mem track
 *   @param    PA         - Physical address of granule
 *   @return   Returns the granule for given PA from NS mem track
**/
val_host_granule_ts *val_host_find_granule(uint64_t PA)
{
    val_host_granule_ts *find = head;

    while (find != NULL)
    {
        if (find->PA == PA)
            return find;
        find = find->next;
    }

    return NULL;
}

/**
 *   @brief    Remove granule from given mem track list
 *   @param    gran_list_head      - head pointer of granule list
 *   @param    PA                  - Physical address of granule
 *   @return   Returns the removed granule from mem track list
**/
val_host_granule_ts *val_host_remove_granule(val_host_granule_ts **gran_list_head, uint64_t PA)
{
    val_host_granule_ts *current = *gran_list_head, *prev = NULL, *temp = NULL;

    temp = mem_track[0].gran_type.ns;

    if ((current != NULL) && (current->PA == PA))
    {
        /* If its NS mem_track list update head */
        if (temp == *gran_list_head)
        {
            head = current->next;
            mem_track[0].gran_type.ns = head;
            current->next = NULL;
            return current;
        } else {
            prev = current;
            current = current->next;
            *gran_list_head = current;
            prev->next = NULL;
            return prev;
        }
    }

    while (NULL != current)
    {
        if (current->PA != PA)
        {
            prev = current;
            current = current->next;
        } else {
            break;
        }
    }

    if (current == NULL)
        return NULL;
    else if (current->PA == PA)
    {
        prev->next = current->next;
        if (mem_track[0].gran_type.ns == *gran_list_head)
        {
            if (current->next == NULL)
                tail = prev;
        }
        current->next = NULL;
        return current;
    } else {
        return NULL;
    }

    return NULL;

}

/**
 *   @brief    Rollback mem_track state update
 *   @param    rd                - Realm RD
 *   @param    PA                - Physical address of granule
 *   @param    ipa               - IPA Address
 *   @param    level             - RTT level
 *   @param    state             - state of granule
 *   @param    gran_list_state   - granule list state
 *   @return   void
**/
void val_host_update_destroy_granule_state(uint64_t rd, uint64_t PA,
                                       uint64_t ipa, uint64_t level,
                           uint32_t state, uint32_t gran_list_state)
{
    val_host_granule_ts *node = NULL;
    int i;

    if (state == GRANULE_UNDELEGATED)
    {
        node = val_host_find_granule(PA);
        if (node != NULL)
        {
            if (node->is_granule_sliced == 0)
            {
                node = val_host_remove_granule(&mem_track[0].gran_type.ns, PA);
                val_host_mem_free((void *)node->PA);
                val_host_mem_free(node);
                return;
            } else if (node->is_granule_sliced == 1) {
                     node->state = state;
            }
        }
        return;
    }

    if (gran_list_state != GRANULE_REC)
    {
        current_realm = val_host_get_curr_realm(rd);
    }

    switch (gran_list_state)
    {
        case GRANULE_RTT:
            node = val_host_remove_rtt_granule(&mem_track[current_realm].gran_type.rtt, ipa, level);
            node->state = state;
            val_host_add_granule(state, node->PA, node);
            break;
        case GRANULE_DATA:
            node = val_host_remove_data_granule(&mem_track[current_realm].gran_type.data, ipa);
            node->state = state;
            val_host_add_granule(state, node->PA, node);
            break;

        case GRANULE_REC:
            for (i = 1; i < VAL_HOST_MAX_REALMS; i++)
            {
                node = val_host_remove_granule(&mem_track[i].gran_type.rec, PA);
                if (node != NULL)
                {
                    node->state = state;
                    val_host_add_granule(state, PA, node);
                    break;
                }
            }
            break;

        case GRANULE_RD:
            node = val_host_remove_granule(&mem_track[current_realm].gran_type.rd, PA);
            node->state = state;
            val_host_add_granule(state, PA, node);
            mem_track[current_realm].rd = 0x00000000FFFFFFFF;
            break;

        case GRANULE_UNPROTECTED:
            node = val_host_remove_granule(&mem_track[current_realm].gran_type.valid_ns, PA);
            node->state = GRANULE_UNDELEGATED;
            val_host_add_granule(state, PA, node);
            break;
    }
}

/**
 *   @brief    Remove data granule from data list and add to the NS mem_track
 *   @param    gran_list_head      - Data granule which needs to remove from data list
 *                                   and add to NS mem track
 *   @param    ipa                 - IPA which needs to remove from data list
 *   @return   Returns the node from data list
**/
val_host_granule_ts *val_host_remove_data_granule(val_host_granule_ts **gran_list_head,
                                                                          uint64_t ipa)
{
    val_host_granule_ts *current = *gran_list_head, *prev = NULL, *temp = NULL;

    temp = mem_track[0].gran_type.ns;
    if ((current != NULL) && (current->ipa == ipa))
    {
        if (temp == *gran_list_head)
        {
            head = current->next;
            mem_track[0].gran_type.ns = head;
            current->next = NULL;
            return current;
        } else {
            prev = current;
            current = current->next;
            *gran_list_head = current;
            prev->next = NULL;
            return prev;
        }
    }

    while (NULL != current)
    {
        if (current->ipa != ipa)
        {
            prev = current;
            current = current->next;
        } else {
            break;
        }
    }

    if (current == NULL)
        return NULL;
    else if (current->ipa == ipa)
    {
        prev->next = current->next;
        if (mem_track[0].gran_type.ns == *gran_list_head)
        {
            if (current->next == NULL)
                tail = prev;
        }
        current->next = NULL;
        return current;
    } else {
        return NULL;
    }

    return NULL;
}

/**
 *   @brief    Remove data granule from data list and add to the NS mem_track
 *   @param    gran_list_head      - Data granule which needs to remove from data list
 *                                   and add to NS mem track
 *   @param    ipa                 - IPA which needs to remove from data list
 *   @return   Returns the node from data list
**/
val_host_granule_ts *val_host_remove_rtt_granule(val_host_granule_ts **gran_list_head,
                                                         uint64_t ipa, uint64_t level)
{
    val_host_granule_ts *current = *gran_list_head, *prev = NULL, *temp = NULL;

    temp = mem_track[0].gran_type.ns;
    if ((current != NULL) && (current->level == level) && (current->ipa == ipa))
    {
        if (temp == *gran_list_head)
        {
            head = current->next;
            mem_track[0].gran_type.ns = head;
            current->next = NULL;
            return current;
        } else {
            prev = current;
            current = current->next;
            *gran_list_head = current;
            prev->next = NULL;
            return prev;
        }
    }

    while (NULL != current)
    {
        if ((current->level != level) || (current->ipa != ipa))
        {
            prev = current;
            current = current->next;
        } else {
            break;
        }
    }

    if (current == NULL)
        return NULL;
    else if ((current->level == level) && (current->ipa == ipa))
    {
        prev->next = current->next;
        if (mem_track[0].gran_type.ns == *gran_list_head)
        {
            if (current->next == NULL)
                tail = prev;
        }
        current->next = NULL;
        return current;
    } else {
        return NULL;
    }

    return NULL;
}

/**
 *   @brief    Destroy rtt levels
 *   @param    rtt_level      - RTT level to destroy
 *   @param    current_realm  - current realm index in mem track
 *   @return   SUCCESS/FAILURE
**/
uint64_t val_host_destroy_rtt_levels(uint64_t rtt_level, int current_realm)
{
    val_host_granule_ts *curr_gran = NULL, *next_gran = NULL;
    uint64_t ret;
    val_host_rtt_destroy_ts rtt_destroy;

    curr_gran = mem_track[current_realm].gran_type.rtt;
    while (curr_gran != NULL)
    {
        next_gran = curr_gran->next;
        if (curr_gran->level == rtt_level)
        {

            ret = val_host_rmi_rtt_destroy(curr_gran->rd,
                                           curr_gran->ipa, curr_gran->level, &rtt_destroy);
            if (ret)
            {
                LOG(ERROR, "\trealm_rtt_destroy failed, rtt=0x%x, ret=0x%x\n", curr_gran->ipa, ret);
                return VAL_ERROR;
            }
            ret = val_host_rmi_granule_undelegate(curr_gran->PA);
            if (ret)
            {
                LOG(ERROR, "\tval_rmi_granule_undelegate failed, rtt=0x%x, ret=0x%x\n",
                                                                   curr_gran->PA, ret);
                return VAL_ERROR;
            }

            curr_gran = next_gran;
        } else {
            curr_gran = next_gran;
        }
    }
    return VAL_SUCCESS;
}

/**
 *   @brief    Rollback the changes
 *   @param    void
 *   @return   SUCCESS/FAILURE
**/
uint64_t val_host_postamble(void)
{
    int i;
    uint64_t ret;
    val_host_granule_ts *curr_gran = NULL, *next_gran = NULL;

    for (i = 1 ; i < VAL_HOST_MAX_REALMS ; i++)
    {
        if (mem_track[i].rd != 0x00000000FFFFFFFF)
        {
            ret = val_host_realm_destroy((uint64_t)mem_track[i].rd);
            if (ret)
            {
                LOG(ERROR, "\tval_host_realm_destroy failed, ret=0x%x\n", ret, 0);
                return VAL_ERROR;
            }
        }
    }

    //Undelegate and free all other granules in NS mem_track
    curr_gran = mem_track[0].gran_type.ns;
    while (curr_gran != NULL)
    {
        if (curr_gran->state == GRANULE_DELEGATED)
        {
            next_gran = curr_gran->next;
            ret = val_host_rmi_granule_undelegate(curr_gran->PA);
            if (ret)
            {
                LOG(ERROR, "\tgranule undelegation failed, pa=0x%x, ret=0x%x\n",
                                                            curr_gran->PA, ret);
                return VAL_ERROR;
            }
            curr_gran = next_gran;
        } else {
            curr_gran = curr_gran->next;
        }
    }

    //Free remaining memory from list
    val_host_granule_ts *node_temp1 = NULL;

    curr_gran = mem_track[0].gran_type.ns;
    if (curr_gran == NULL)
    {
        return VAL_SUCCESS;
    } else {
        while (curr_gran != NULL)
        {
            if (curr_gran->state == GRANULE_UNDELEGATED)
            {
                next_gran = curr_gran->next;
                node_temp1 = val_host_remove_granule(&mem_track[0].gran_type.ns, curr_gran->PA);
                val_host_mem_free((void *)node_temp1->PA);
                val_host_mem_free(node_temp1);
                curr_gran = next_gran;

            } else {
                curr_gran = curr_gran->next;
            }
        }
    }

    return VAL_SUCCESS;
}

/**
 *   @brief    Destroy Realm
 *   @param    rd      -  Realm RD granule address
 *   @return   SUCCESS/FAILURE
**/
uint32_t val_host_realm_destroy(uint64_t rd)
{
    uint64_t ret;
    val_host_granule_ts *curr_gran = NULL, *next_gran = NULL;
    val_host_data_destroy_ts data_destroy;
    current_realm = val_host_get_curr_realm(rd);
    uint64_t top;

    /* For each REC - Destroy, undelegate */
    curr_gran = mem_track[current_realm].gran_type.rec;
    while (curr_gran != NULL)
    {
        next_gran = curr_gran->next;
        ret = val_host_rmi_rec_destroy(curr_gran->PA);
        if (ret)
        {
            LOG(ERROR, "\tREC destroy failed, rec=0x%x, ret=0x%x\n", curr_gran->PA, ret);
            return VAL_ERROR;
        }

        ret = val_host_rmi_granule_undelegate(curr_gran->PA);
        if (ret)
        {
            LOG(ERROR, "\trec undelegation failed, rec=0x%x, ret=0x%x\n", curr_gran->PA, ret);
            return VAL_ERROR;
        }
       curr_gran = next_gran;
    }

    // Destroy and undelegate realm protected granules
    curr_gran = mem_track[current_realm].gran_type.data;
    while (curr_gran != NULL)
    {
        next_gran = curr_gran->next;
        if (curr_gran->is_granule_sliced == 1)
        {
            ret = val_host_rmi_data_destroy(curr_gran->rd, curr_gran->ipa, &data_destroy);
            if (ret)
            {
                LOG(ERROR, "\tData destroy failed, data=0x%x, ret=0x%x\n", curr_gran->PA, ret);
                return VAL_ERROR;
            }

            ret = val_host_rmi_granule_undelegate(curr_gran->PA);
            if (ret)
            {
                LOG(ERROR, "\tdata undelegation failed, pa=0x%x, ret=0x%x\n", curr_gran->PA, ret);
                return VAL_ERROR;
            }
        }
        curr_gran = next_gran;
    }

    curr_gran = mem_track[current_realm].gran_type.data;
    while (curr_gran != NULL)
    {
        next_gran = curr_gran->next;
        if (curr_gran->is_granule_sliced == 0)
        {
            ret = val_host_rmi_data_destroy(curr_gran->rd, curr_gran->ipa, &data_destroy);
            if (ret)
            {
                LOG(ERROR, "\tData destroy failed, data=0x%x, ret=0x%x\n", curr_gran->PA, ret);
                return VAL_ERROR;
            }

            ret = val_host_rmi_granule_undelegate(curr_gran->PA);
            if (ret)
            {
                LOG(ERROR, "\tdata undelegation failed, pa=0x%x, ret=0x%x\n", curr_gran->PA, ret);
                return VAL_ERROR;
            }
            curr_gran = next_gran;
        } else {
            curr_gran = next_gran;
        }
    }

    // Unmap unprotected granules
    curr_gran = mem_track[current_realm].gran_type.valid_ns;
    while (curr_gran != NULL)
    {
        next_gran = curr_gran->next;
        ret = val_host_rmi_rtt_unmap_unprotected(curr_gran->rd, curr_gran->ipa,
                                                       curr_gran->level, &top);
        if (ret)
        {
            LOG(ERROR, "\tval_rmi_rtt_unmap_unprotected failed, ipa=0x%x, ret=0x%x\n",
                                                                 curr_gran->ipa, ret);
            return VAL_ERROR;
        }
        curr_gran = next_gran;
    }

    // Destroy leaf rtt hirerachy
    if (val_host_destroy_rtt_levels(3, current_realm))
        return VAL_ERROR;
    if (val_host_destroy_rtt_levels(2, current_realm))
        return VAL_ERROR;
    if (val_host_destroy_rtt_levels(1, current_realm))
        return VAL_ERROR;

    // RD destroy, undelegate and free
    ret = val_host_rmi_realm_destroy(mem_track[current_realm].rd);
    if (ret)
    {
        LOG(ERROR, "\tRealm destroy failed, rd=0x%x, ret=0x%x\n", mem_track[current_realm].rd, ret);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}
