 /*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_rmi.h"
#include "command_common_host.h"
#include "val_host_realm.h"
#include "val_host_command.h"
#include "val_host_pcie.h"
#include "val_host_doe.h"
#include "pal_common_support.h"
#include "val_timer.h"
#include "val_crypto.h"
#include "mbedtls/debug.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/oid.h"
#include "val_host_helpers.h"
#include "val_host_sro.h"

#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512 * L3_SIZE)
#define L1_SIZE (512 * L2_SIZE)
#define MAP_LEVEL 3
#define IPA_WIDTH 40
#define IPA_ADDR_PROTECTED_ASSIGNED_RAM PAGE_SIZE
#define IPA_ADDR_PROTECTED_ASSIGNED_EMPTY (4 * PAGE_SIZE)
#define IPA_ADDR_PROTECTED_UNASSIGNED_RAM 0
#define IPA_ADDR_PROTECTED_UNASSIGNED_EMPTY (3 * PAGE_SIZE)
#define IPA_ADDR_DESTROYED (2 * PAGE_SIZE)
#define IPA_ADDR_UNPROTECTED (1UL << (IPA_WIDTH - 1))
#define IPA_ADDR_UNPROTECTED_UNASSIGNED (IPA_ADDR_UNPROTECTED + PAGE_SIZE)
#define IPA_ADDR_PROTECTED_AUX_ASSIGNED (7 * PAGE_SIZE)

uint64_t val_host_tracking_none_prep_sequence(val_host_tracking_test_region_ts *region)
{
    const uint64_t base = PLATFORM_TRACKING_TEST_REGION_BASE;
    val_smc_param_ts ret;

    region->base = base;
    region->delegated_start = base;
    region->delegated_top = base;
    region->state_changed = false;

    ret = val_host_rmi_granule_tracking_get(base, base + PAGE_SIZE);
    if (ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "Tracking test region TRACKING_GET failed, ret=0x%lx\n",
            ret.x0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if ((ret.x1 != RMI_MEM_CATEGORY_CONVENTIONAL) ||
        (ret.x3 < base + PAGE_SIZE))
    {
        LOG(ERROR, "Tracking test region is not conventional DRAM: "
            "category=0x%lx, top=0x%lx\n", ret.x1, ret.x3);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if ((ret.x2 == RMI_TRACKING_FINE) ||
        (ret.x2 == RMI_TRACKING_INTERMEDIATE))
    {
        LOG(TEST, "Tracking test region is FINE/INTERMEDIATE\n");
        return VAL_SKIP_CHECK;
    }

    if (ret.x2 == RMI_TRACKING_COARSE)
    {
        if (val_host_rmi_granule_tracking_set(base,
                RMI_MEM_CATEGORY_CONVENTIONAL, RMI_TRACKING_NONE) != RMI_SUCCESS)
        {
            LOG(ERROR, "Could not set tracking test region to NONE\n");
            return VAL_TEST_PREP_SEQ_FAILED;
        }
        region->state_changed = true;
    }
    else if (ret.x2 != RMI_TRACKING_NONE)
    {
        LOG(ERROR, "Tracking test region has invalid state 0x%lx\n", ret.x2);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    ret = val_host_rmi_granule_tracking_get(base, base + PAGE_SIZE);
    if ((ret.x0 != RMI_SUCCESS) ||
        (ret.x1 != RMI_MEM_CATEGORY_CONVENTIONAL) ||
        (ret.x2 != RMI_TRACKING_NONE) ||
        (ret.x3 < base + PAGE_SIZE))
    {
        LOG(ERROR, "Tracking test region is not NONE after setup\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return VAL_SUCCESS;
}

uint32_t val_host_tracking_none_cleanup(val_host_tracking_test_region_ts *region)
{
    if (region->state_changed)
    {
        if (val_host_rmi_granule_tracking_set(region->base,
                RMI_MEM_CATEGORY_CONVENTIONAL, RMI_TRACKING_COARSE) != RMI_SUCCESS)
        {
            LOG(ERROR, "Could not restore tracking test region to COARSE\n");
            return VAL_ERROR;
        }
        region->state_changed = false;
    }

    return VAL_SUCCESS;
}

uint32_t val_host_realm_create_common(val_host_realm_ts *realm)
{
    val_host_realm_params_ts *params;
    uint64_t ret, i, j;

    /* Allocate and delegate RD */
    realm->rd = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!realm->rd)
    {
        LOG(ERROR, "Failed to allocate memory for rd\n");
    } else {
        ret = val_host_rmi_granule_range_delegate(realm->rd, realm->rd + PAGE_SIZE);
        if (ret)
        {
            LOG(ERROR, "rd delegation failed, rd=0x%x, ret=0x%x\n", realm->rd, ret);
            goto free_rd;
        }
    }

    /* Allocate and delegate RTT */
    realm->rtt_l0_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!realm->rtt_l0_addr)
    {
        LOG(ERROR, "Failed to allocate memory for rtt_addr\n");
        goto undelegate_rd;
    } else {
        ret = val_host_rmi_granule_range_delegate(
            realm->rtt_l0_addr, realm->rtt_l0_addr + PAGE_SIZE);
        if (ret)
        {
            LOG(ERROR, "rtt delegation failed, rtt_addr=0x%x, ret=0x%x\n",
                realm->rtt_l0_addr, ret);
            goto free_rtt;
        }
    }

    /* If realm is configured to use RTT tree per plane, Allocate and delegate
     * auxiliary RTTs */
    if (VAL_EXTRACT_BITS(realm->flags1, 0, 0) && realm->num_aux_planes > 0)
    {
        for (i = 0; i < realm->num_aux_planes; i++)
        {
            realm->rtt_aux_l0_addr[i] = (uint64_t)val_host_mem_alloc(
           (realm->num_s2_sl_rtts * PAGE_SIZE), (realm->num_s2_sl_rtts * PAGE_SIZE));
            if (!realm->rtt_aux_l0_addr[i])
            {
                LOG(ERROR, "Failed to allocate memory for rtt_addr\n");
                goto undelegate_rd;
            } else {
                for (j = 0; j < realm->num_s2_sl_rtts; j++)
                {
                    ret = val_host_rmi_granule_range_delegate(
                        realm->rtt_aux_l0_addr[i] + (j * PAGE_SIZE),
                        realm->rtt_aux_l0_addr[i] + ((j + 1) * PAGE_SIZE));
                    if (ret)
                    {
                        LOG(ERROR, "rtt delegation failed, rtt_addr=0x%x, ret=0x%x\n",
                        realm->rtt_aux_l0_addr[i], ret);
                        goto free_rtt;
                    }
                }
            }
        }
    }


    /* Allocate memory for params */
    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (params == NULL)
    {
        LOG(ERROR, "Failed to allocate memory for params\n");
        goto undelegate_rtt;
    }

    val_memset(params, 0, PAGE_SIZE);

    /* Populate params */
    params->rtt_base = realm->rtt_l0_addr;
    params->hash_algo = realm->hash_algo;
    params->s2sz = realm->s2sz;
    params->rtt_level_start = (int64_t)realm->s2_starting_level;
    params->rtt_num_start = realm->num_s2_sl_rtts;
    params->num_aux_planes = realm->num_aux_planes;
    params->flags0 = realm->flags;
    params->flags1 = realm->flags1;
    for (i = 0; i < realm->num_aux_planes; i++)
        params->aux_rtt_base[i] = realm->rtt_aux_l0_addr[i];
    /* RealmParams structure takes the number of breakpoints, minus one */
    params->num_bps = realm->num_bps + 1;
    params->num_wps = realm->num_wps + 1;

#ifdef RMM_V_1_1
    if (realm->num_aux_planes == 0)
        params->flags1 |= VAL_REALM_FLAG_RTT_TREE_PP;
#endif

    /* Create realm */
    if (val_host_rmi_realm_create(realm->rd, (uint64_t)params))
    {
        LOG(ERROR, "Realm create failed\n");
        goto free_params;
    }

    /* Free params */
    val_host_mem_free(params);
    return VAL_SUCCESS;

free_params:
    val_host_mem_free(params);

undelegate_rtt:
    ret = val_host_rmi_granule_range_undelegate(realm->rtt_l0_addr, realm->rtt_l0_addr + PAGE_SIZE);
    if (ret)
    {
        LOG(WARN, "rtt undelegation failed, rtt_addr=0x%x, ret=0x%x\n", realm->rtt_l0_addr, ret);
    }

free_rtt:
     val_host_mem_free((void *)realm->rtt_l0_addr);

undelegate_rd:
    ret = val_host_rmi_granule_range_undelegate(realm->rd, realm->rd + PAGE_SIZE);
    if (ret)
    {
        LOG(WARN, "rd undelegation failed, rd=0x%x, ret=0x%x\n", realm->rd, ret);
    }
free_rd:
     val_host_mem_free((void *)realm->rd);

    return VAL_ERROR;
}

uint32_t val_host_rec_create_common(val_host_realm_ts *realm, val_host_rec_params_ts *params)
{
    val_host_rec_params_ts   *rec_params;
    val_host_rec_create_flags_ts rec_create_flags;
    uint64_t ret, i, mpidr = 0x0;

    if (realm->rec_count > VAL_MAX_REC_COUNT)
    {
        LOG(ERROR, "max supported recs are VAL_MAX_REC_COUNT\n");
        return VAL_ERROR;
    }

    val_memset(&rec_create_flags, 0x0, sizeof(rec_create_flags));

    /* Allocate memory for rec_params */
    rec_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (rec_params == NULL)
    {
        LOG(ERROR, "Failed to allocate memory for rec_params\n");
        return VAL_ERROR;
    }
    val_memset(rec_params, 0x0, PAGE_SIZE);

    /* Populate rec_params */

    for (i = 0; i < (sizeof(rec_params->gprs)/sizeof(rec_params->gprs[0])); i++)
    {
        rec_params->gprs[i] = 0x0;
    }

    rec_params->pc = params->pc;
    rec_create_flags.runnable = RMI_RUNNABLE;
    val_memcpy(&rec_params->flags, &rec_create_flags, sizeof(rec_create_flags));

    for (i = 0; i < realm->rec_count; i++, mpidr++)
    {
        rec_params->mpidr = mpidr;
        /* Allocate memory for run object */
        realm->run[i] = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
        if (!realm->run[i])
        {
            LOG(ERROR, "Failed to allocate memory for run[%d]\n", i);
            goto free_rec_params;
        }
        val_memset((void *)realm->run[i], 0x0, PAGE_SIZE);

        /* Allocate and delegate REC */
        realm->rec[i] = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
        if (!realm->rec[i])
        {
            LOG(ERROR, "Failed to allocate memory for REC\n");
            goto free_rec_params;
        } else {
            ret = val_host_rmi_granule_range_delegate(realm->rec[i], realm->rec[i] + PAGE_SIZE);
            if (ret)
            {
                LOG(ERROR, "rec delegation failed, rec=0x%x, ret=0x%x\n", realm->rec[i], ret);
                goto free_rec_params;
            }
        }

        /* Create REC  */
        ret = val_host_rmi_rec_create(realm->rd, realm->rec[i], (uint64_t)rec_params);
        if (ret)
        {
            LOG(ERROR, "REC create failed, ret=0x%x\n", ret);
            goto free_rec_params;
        }
    }

    /* Free rec_params */
    val_host_mem_free(rec_params);
    return VAL_SUCCESS;

free_rec_params:
    while (i > 0)
    {
        ret = val_host_rmi_granule_range_undelegate(realm->rec[i], realm->rec[i] + PAGE_SIZE);
        if (ret)
        {
            LOG(WARN, "rec undelegation failed, rec=0x%x, ret=0x%x\n", realm->rec[i], ret);
        }

        val_host_mem_free((void *)realm->rec[i]);
        val_host_mem_free((void *)realm->run[i]);

        i--;
        if (i == 0)
        {
            break;
        }
    }

    val_host_mem_free(rec_params);
    return VAL_ERROR;
}

uint64_t g_delegated_prep_sequence(void)
{
    return val_host_delegate_granule();
}

uint64_t g_undelegated_prep_sequence(void)
{
    return val_host_undelegate_granule();
}

uint64_t g_unaligned_prep_sequence(uint64_t gran)
{
    return gran + 1;
}

uint64_t g_dev_mem_prep_sequence(void)
{
    return (uint64_t)val_get_ns_uart_base();
}

uint64_t g_outside_of_permitted_pa_prep_sequence(void)
{
    uint64_t pa_range = val_get_pa_range_supported();
    return (0x1ull << pa_range) + PAGE_SIZE;
}

uint64_t g_secure_prep_sequence(void)
{
    return PLATFORM_SECURE_PAS_BASE;
}

uint64_t g_data_prep_sequence(uint64_t rd, uint64_t ipa)
{
    /* Pick an address that has rtte.state = ASSIGNED */
    if (create_mapping(ipa, true, rd))
    {
        LOG(ERROR, "Couldn't create the assigned protected mapping\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    uint64_t data = g_delegated_prep_sequence();
    if (data == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    uint64_t src = g_undelegated_prep_sequence();
    if (src == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    uint64_t flags = RMI_NO_MEASURE_CONTENT;

    if (val_host_rmi_rtt_data_map_init(rd, data, ipa, src, flags))
    {
        LOG(ERROR, "Couldn't complete the assigned protected mapping\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    return data;
}

uint64_t ipa_outside_of_permitted_ipa_prep_sequence(void)
{
    /* Pick a granule aligned address that falls outside of the Realm's IPA space */

    uint64_t val;
    val_host_rmi_features(0, &val);
    uint64_t ipa_width = VAL_EXTRACT_BITS(val, 0, 8);

    return (0x1ull << ipa_width) + PAGE_SIZE;
}

uint64_t ipa_protected_unmapped_prep_sequence(void)
{
    /* Pick an address that is not mapped in the valid Realm
     * i.e. return ipa = L2_ENTRY_SIZE (1st entry of 2nd L2 table)
     */
    return L2_SIZE;
}

uint64_t ipa_protected_assigned_ram_prep_sequence(uint64_t rd1)
{
    /* Pick an address that has rtte.state = ASSIGNED */
    if (create_mapping(IPA_ADDR_PROTECTED_ASSIGNED_RAM, true, rd1))
    {
        LOG(ERROR, "Couldn't create the assigned protected mapping\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    uint64_t rd = rd1;
    uint64_t data = g_delegated_prep_sequence();
    if (data == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    uint64_t src = g_undelegated_prep_sequence();
    if (src == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    uint64_t flags = RMI_NO_MEASURE_CONTENT;

    if (val_host_rmi_rtt_data_map_init(rd, data, IPA_ADDR_PROTECTED_ASSIGNED_RAM, src, flags))
    {
        LOG(ERROR, "Couldn't complete the assigned protected mapping\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return IPA_ADDR_PROTECTED_ASSIGNED_RAM;
}

uint64_t ipa_protected_assigned_empty_prep_sequence(uint64_t rd)
{
    uint64_t data;

    /* Pick an address that has rtte.state = ASSIGNED */
    if (create_mapping(IPA_ADDR_PROTECTED_ASSIGNED_EMPTY, false, rd))
    {
        LOG(ERROR, "Couldn't create the assigned protected mapping\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    data = g_delegated_prep_sequence();
    if (data == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    val_host_rmi_rtt_prot_map_flags_ts data_map_flags;
    uint64_t flags = 0;

    val_memset(&data_map_flags, 0x0, sizeof(data_map_flags));
    data_map_flags.oaddr_type = RMI_ADDR_TYPE_SINGLE;
    val_memcpy(&flags, &data_map_flags, sizeof(data_map_flags));

    if (val_host_rmi_rtt_data_map(rd, IPA_ADDR_PROTECTED_ASSIGNED_EMPTY,
                                  IPA_ADDR_PROTECTED_ASSIGNED_EMPTY + PAGE_SIZE,
                                  flags,
                                  RMI_PAGE_L3 |
                                  (1UL << 2) |
                                  ADDR_ALIGN_DOWN(data, PAGE_SIZE) |
                                  ((uint64_t)RMI_OP_MEM_DELEGATED << 62)))
    {
        LOG(ERROR, "Couldn't complete the assigned protected mapping\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return IPA_ADDR_PROTECTED_ASSIGNED_EMPTY;
}

uint64_t ipa_protected_destroyed_prep_sequence(uint64_t rd1)
{
    val_host_rmi_rtt_unmap_flags_ts data_unmap_flags;
    uint64_t unmap_flags = 0;

    /* Pick an address that has rtte.state = DESTROYED*/
    if (create_mapping(IPA_ADDR_DESTROYED, true, rd1))
    {
        LOG(ERROR, "Couldn't create the assigned protected mapping\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    uint64_t rd = rd1;
    uint64_t data = g_delegated_prep_sequence();
    if (data == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    uint64_t src = g_undelegated_prep_sequence();
    if (src == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    uint64_t flags = RMI_NO_MEASURE_CONTENT;

    if (val_host_rmi_rtt_data_map_init(rd, data, IPA_ADDR_DESTROYED, src, flags))
    {
        LOG(ERROR, "Couldn't complete the assigned protected mapping\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    val_memset(&data_unmap_flags, 0x0, sizeof(data_unmap_flags));
    data_unmap_flags.oaddr_type = RMI_ADDR_TYPE_SINGLE;
    val_memcpy(&unmap_flags, &data_unmap_flags, sizeof(data_unmap_flags));

    if (val_host_rmi_rtt_data_unmap(rd, IPA_ADDR_DESTROYED,
                                    IPA_ADDR_DESTROYED + PAGE_SIZE,
                                    unmap_flags, 0))
    {
        LOG(ERROR, "Couldn't complete the destroyed mapping\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return IPA_ADDR_DESTROYED;
}

uint64_t ipa_unprotected_unmapped_prep_sequence(void)
{
    /* Pick an address that is not mapped in the valid Realm and is in Unprotected IPA range
     * i.e. return ipa = 2**IPA_WIDTH + L2_BLOCK_SIZE (1st entry of 2nd Unprotected L2 table)
     */
    return ((1ull << (40 - 1)) + VAL_RTT_L2_BLOCK_SIZE);
}

uint64_t ipa_unprotected_assinged_prep_sequence(uint64_t rd1)
{
    val_host_rmi_rtt_unprot_map_flags_ts map_flags = {0};
    uint64_t flags = 0;

    /* Pick an address that is in unprotected range and has rtte.state != UNASSIGNED */
    if (create_mapping(IPA_ADDR_UNPROTECTED, false, rd1))
    {
        LOG(ERROR, "Couldn't create the unprotected mapping\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    uint64_t ns = g_undelegated_prep_sequence();
    if (ns == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    uint64_t desc = (ns | ATTR_NORMAL_WB_WA_RA | ATTR_STAGE2_AP_RW);

    map_flags.oaddr_type = RMI_ADDR_TYPE_SINGLE;
    map_flags.memattr = VAL_EXTRACT_BITS(desc, ATTR_STAGE2_MEMATTR_SHIFT,
                                        ATTR_STAGE2_MEMATTR_SHIFT + 2);
    map_flags.s2ap =
        (VAL_EXTRACT_BITS(desc, ATTR_STAGE2_AP_SHIFT,
                         ATTR_STAGE2_AP_SHIFT) << 1) |
        VAL_EXTRACT_BITS(desc, (ATTR_STAGE2_AP_SHIFT + 1),
                        (ATTR_STAGE2_AP_SHIFT + 1));
    val_memcpy(&flags, &map_flags, sizeof(map_flags));

    if (val_host_rmi_rtt_unprot_map(rd1, IPA_ADDR_UNPROTECTED,
                                    IPA_ADDR_UNPROTECTED + PAGE_SIZE,
                                    flags,
                                    RMI_PAGE_L3 |
                                    (1UL << 2) |
                                    ADDR_ALIGN_DOWN(ns, PAGE_SIZE) |
                                    ((uint64_t)RMI_OP_MEM_UNDELEGATED << 62)))
    {
        LOG(ERROR, "Couldn't complete the unprotected mapping\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    return IPA_ADDR_UNPROTECTED;
}

uint64_t ipa_unprotected_unassigned_prep_sequence(uint64_t rd1)
{
    /* Pick an address that is within unprotected range and has rtte.state = UNASSIGNED */
    if (create_mapping(IPA_ADDR_UNPROTECTED_UNASSIGNED, false, rd1))
    {
        LOG(ERROR, "Unassigned unprotected ipa mapping creation failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    return IPA_ADDR_UNPROTECTED_UNASSIGNED;
}

uint64_t ipa_protected_unassigned_ram_prep_sequence(uint64_t rd)
{
    if (create_mapping(IPA_ADDR_PROTECTED_UNASSIGNED_RAM, true, rd))
    {
        LOG(ERROR, "Protected Unassinged ipa mapping creation failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    return IPA_ADDR_PROTECTED_UNASSIGNED_RAM;
}

uint64_t ipa_protected_unassigned_empty_prep_sequence(uint64_t rd)
{
    if (create_mapping(IPA_ADDR_PROTECTED_UNASSIGNED_EMPTY, false, rd))
    {
        LOG(ERROR, "Protected Unassinged ipa mapping creation failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    return IPA_ADDR_PROTECTED_UNASSIGNED_EMPTY;
}

uint64_t ipa_protected_aux_assigned_prep_sequence(uint64_t rd, uint64_t rtt_index)
{
    /* Pick an address that has rtte.state = ASSIGNED */
    if (create_mapping(IPA_ADDR_PROTECTED_AUX_ASSIGNED, true, rd))
    {
        LOG(ERROR, "RTT creation failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    uint64_t data = g_delegated_prep_sequence();
    if (data == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    uint64_t src = g_undelegated_prep_sequence();
    if (src == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    uint64_t flags = RMI_NO_MEASURE_CONTENT;

    if (val_host_rmi_data_create(rd, data, IPA_ADDR_PROTECTED_AUX_ASSIGNED, src, flags))
    {
        LOG(ERROR, "DATA CREATE failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (val_host_create_aux_mapping(rd, IPA_ADDR_PROTECTED_AUX_ASSIGNED, rtt_index))
    {
        LOG(ERROR, "Couldn't create the assigned protected mapping\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (val_host_rmi_rtt_aux_map_protected(rd, IPA_ADDR_PROTECTED_AUX_ASSIGNED, rtt_index).x0)
    {
        LOG(ERROR, "AUX MAP PROTECTED failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return IPA_ADDR_PROTECTED_AUX_ASSIGNED;
}

uint64_t g_pa_in_lpa2_range_prep_sequence(void)
{
    return (0x1ULL << 48) + PAGE_SIZE;
}


uint64_t g_vdev_new_prep_sequence(val_host_pdev_ts *pdev_dev, val_host_vdev_ts *vdev_dev,
                                  val_host_realm_ts *realm)
{
    uint64_t ret, req_addr, resp_addr, vdev;
    val_host_rmi_vdev_params_ts *vdev_params;
    val_smc_param_ts args;
    uint64_t j;
    val_host_realm_flags_ts realm_flags;
    val_host_public_key_params_ts *pubkey_params;
    int rc;
    uint8_t public_key_algo;

    /* Check whether RMM supports Device Assignment. */
    if (!val_host_rmm_supports_da()) {
        LOG(ERROR, "DA feature not supported\n");
        goto exit;
    }

    pdev_dev->pdev = g_pdev_new_prep_sequence(pdev_dev);
    if (pdev_dev->pdev == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    /* Allocate buffer to cache device certificate */
    pdev_dev->cert_slot_id = 0;
    pdev_dev->cert_chain = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_PDEV_CERT_LEN_MAX);
    pdev_dev->cert_chain_len = 0;
    if (pdev_dev->cert_chain == NULL) {
        goto exit;
    }

    /* Allocate buffer to store extracted public key */
    pdev_dev->public_key = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev_dev->public_key == NULL) {
        goto exit;
    }
    pdev_dev->public_key_len = PAGE_SIZE;

    /* Allocate buffer to store public key metadata */
    pdev_dev->public_key_metadata = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev_dev->public_key_metadata == NULL) {
        goto exit;
    }
    pdev_dev->public_key_metadata_len = PAGE_SIZE;

    /* Allocate memory for req addr buffer */
    req_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!req_addr)
    {
        LOG(ERROR, "Failed to allocate memory for req_addr\n");
        goto exit;
    }

    /* Allocate memory for resp_addr buffer */
    resp_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!resp_addr)
    {
        LOG(ERROR, "Failed to allocate memory for resp_addr\n");
        goto exit;
    }

    pdev_dev->dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    pdev_dev->dev_comm_data->enter.req_addr = req_addr;
    pdev_dev->dev_comm_data->enter.rsp_addr = resp_addr;

    if (val_host_dev_communicate(NULL, pdev_dev, NULL, RMI_PDEV_NEEDS_KEY))
    {
        LOG(ERROR, "PDEV communicate failed \n");
        goto exit;
    }

    LOG(TEST, "\n\tPDEV state has been changed to PDEV_NEEDS_KEY\n");

    /* Get public key */
    rc = val_host_get_public_key_from_cert_chain(pdev_dev->cert_chain,
                         pdev_dev->cert_chain_len,
                         pdev_dev->public_key,
                         &pdev_dev->public_key_len,
                         pdev_dev->public_key_metadata,
                         &pdev_dev->public_key_metadata_len,
                         &public_key_algo);
    if (rc != 0) {
        LOG(ERROR, "Get public key failed\n");
        goto exit;
    }

    if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P256) {
        pdev_dev->public_key_sig_algo = RMI_SIG_ECDSA_P256;
    } else if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P384) {
        pdev_dev->public_key_sig_algo = RMI_SIG_ECDSA_P384;
    } else {
        pdev_dev->public_key_sig_algo = RMI_SIG_RSASSA_3072;
    }

    pubkey_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!pubkey_params)
    {
        LOG(ERROR, "\tFailed to allocate memory for pub_key\n");
        goto exit;
    }

    val_memset(pubkey_params, 0, PAGE_SIZE_4K);
    pubkey_params->key_addr = (uint64_t)pdev_dev->public_key;
    pubkey_params->metadata_addr = (uint64_t)pdev_dev->public_key_metadata;
    pubkey_params->key_len = pdev_dev->public_key_len;
    pubkey_params->metadata_len = pdev_dev->public_key_metadata_len;

    args = val_host_rmi_pdev_set_pubkey(pdev_dev->pdev, (uint64_t)pubkey_params);
    if (args.x0)
    {
        LOG(ERROR, "Pdev set key failed %lx\n", args.x0, 0);
        goto exit;
    }

    args = val_host_rmi_pdev_get_state(pdev_dev->pdev);
    if (args.x0)
    {
        LOG(ERROR, "Pdev get state failed\n");
        goto exit;
    }

    /* PDEV state should be RMI_PDEV_HAS_KEY */
    if (args.x1 != RMI_PDEV_HAS_KEY)
    {
        LOG(ERROR, "Pdev state should be PDEV_HAS_KEY\n");
        goto exit;
    }

    LOG(TEST, "\n\tPDEV state has been changed to PDEV_HAS_KEY\n");

    if (val_host_dev_communicate(NULL, pdev_dev, NULL, RMI_PDEV_READY))
    {
        LOG(ERROR, "PDEV communicate failed \n");
        goto exit;
    }

    LOG(TEST, "\n\tPDEV state has been changed to RMI_PDEV_READY\n");

    vdev_dev->pdev_stream = val_host_mem_alloc(
        PAGE_SIZE, sizeof(*vdev_dev->pdev_stream));
    if ((vdev_dev->pdev_stream == NULL) ||
        val_host_pdev_stream_setup(pdev_dev, pdev_dev->root_id,
                                   PLATFORM_OVERRIDE_SMMUV3_BASE,
                                   vdev_dev->pdev_stream))
    {
        LOG(ERROR, "PDEV stream setup failed\n");
        goto exit;
    }

    val_memset(realm, 0, sizeof(*realm));
    val_memset(&realm_flags, 0, sizeof(realm_flags));
    /* Overwrite Realm Parameters */
    realm_flags.da = RMI_FEATURE_TRUE;

    val_host_realm_params(realm);
    realm->vmid = vdev_dev->vmid;
    realm->max_num_vdevs = 2;
    if (vdev_dev->rec_count)
        realm->rec_count = vdev_dev->rec_count;

    val_memcpy(&realm->flags, &realm_flags, sizeof(realm->flags));

    /* Populate realm with one REC*/
    if (val_host_realm_setup(realm, 1))
    {
        LOG(ERROR, "\tRealm setup failed\n");
        goto exit;
    }

    /* Allocate and delegate VDEV */
    vdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!vdev)
    {
        LOG(ERROR, "Failed to allocate memory for vdev\n");
        goto exit;
    } else {
        ret = val_host_rmi_granule_range_delegate(vdev, vdev + PAGE_SIZE);
        if (ret)
        {
            LOG(ERROR, "vdev delegation failed, vdev=0x%x, ret=0x%x", vdev, ret);
            goto exit;
        }
    }

    /* Allocate memory for vdev params */
    vdev_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    if (vdev_params == NULL)
    {
        LOG(ERROR, "Failed to allocate memory for vdev_params\n");
        goto exit;
    }
    val_memset(vdev_params, 0, PAGE_SIZE_4K);

    /* Populate params */
    vdev_params->flags = 0;
    vdev_params->vdev_id = vdev_dev->vdev_id;
    vdev_params->tdi_id = (vdev_dev->num_addr_range != 0U) ? vdev_dev->tdi_id :
                          PCIE_CREATE_BDF_PACKED(pdev_dev->bdf);
    if (vdev_dev->num_addr_range != 0U)
    {
        vdev_params->num_addr_range = vdev_dev->num_addr_range;
        for (j = 0; j < vdev_params->num_addr_range; j++)
            vdev_params->addr_range[j] = vdev_dev->addr_range[j];
    } else {
        vdev_params->num_addr_range = pdev_dev->ncoh_num_addr_range;
        for (j = 0; j < vdev_params->num_addr_range; j++)
            vdev_params->addr_range[j] = pdev_dev->ncoh_addr_range[j];
    }

    /* Create vdev */
    ret = val_host_rmi_vdev_create(realm->rd, pdev_dev->pdev, vdev,
                                   (uint64_t)vdev_params);
    if (ret)
    {
        LOG(ERROR, "Vdev create failed\n");
        goto exit;
    }

    vdev_dev->vdev = vdev;
    vdev_dev->tdi_id = vdev_params->tdi_id;
    vdev_dev->vdev_id = vdev_params->vdev_id;
    vdev_dev->realm_rd = realm->rd;
    vdev_dev->pdev = pdev_dev->pdev;

    args = val_host_rmi_vdev_get_state(vdev);
    if (args.x0)
    {

        LOG(ERROR, "Vdev get state failed ret %d\n", args.x0, 0);
        goto exit;
    }

    /* Check VDEV state is RMI_VDEV_NEW */
    if (args.x1 != RMI_VDEV_NEW)
    {
        LOG(ERROR, "Vdev state should be VDEV_NEW, ret %d\n", args.x1, 0);
        goto exit;
    }

    vdev_dev->dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    vdev_dev->dev_comm_data->enter.req_addr = req_addr;
    vdev_dev->dev_comm_data->enter.rsp_addr = resp_addr;
    vdev_dev->dev_comm_data->enter.status = RMI_DEV_COMM_NONE;
    vdev_dev->dev_comm_data->enter.rsp_len = 0;
    LOG(TEST, "\n\tVDEV Create is successful and VDEV state is VDEV_NEW\n");

    return vdev;
exit:
    return VAL_TEST_PREP_SEQ_FAILED;

}

uint64_t g_pdev_ready_prep_sequence(val_host_pdev_ts *pdev_dev)
{
    uint64_t req_addr, resp_addr;
    val_smc_param_ts args;
    val_host_public_key_params_ts *pubkey_params = NULL;
    int rc;
    uint8_t public_key_algo;

    /* Check whether RMM supports Device Assignment. */
    if (!val_host_rmm_supports_da()) {
        LOG(ERROR, "DA feature not supported\n");
        goto exit;
    }

    if (pdev_dev->pdev == 0U)
    {
        pdev_dev->pdev = g_pdev_new_prep_sequence(pdev_dev);
        if (pdev_dev->pdev == VAL_TEST_PREP_SEQ_FAILED)
            return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Allocate buffer to cache device certificate */
    pdev_dev->cert_slot_id = 0;
    pdev_dev->cert_chain = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_PDEV_CERT_LEN_MAX);
    pdev_dev->cert_chain_len = 0;
    if (pdev_dev->cert_chain == NULL) {
        goto exit;
    }

    /* Allocate buffer to store extracted public key */
    pdev_dev->public_key = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev_dev->public_key == NULL) {
        goto exit;
    }
    pdev_dev->public_key_len = PAGE_SIZE;

    /* Allocate buffer to store public key metadata */
    pdev_dev->public_key_metadata = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev_dev->public_key_metadata == NULL) {
        goto exit;
    }
    pdev_dev->public_key_metadata_len = PAGE_SIZE;

    /* Allocate memory for req addr buffer */
    req_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!req_addr)
    {
        LOG(ERROR, "Failed to allocate memory for req_addr\n");
        goto exit;
    }

    /* Allocate memory for resp_addr buffer */
    resp_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!resp_addr)
    {
        LOG(ERROR, "Failed to allocate memory for resp_addr\n");
        goto exit;
    }

    pdev_dev->dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    pdev_dev->dev_comm_data->enter.req_addr = req_addr;
    pdev_dev->dev_comm_data->enter.rsp_addr = resp_addr;

    if (val_host_dev_communicate(NULL, pdev_dev, NULL, RMI_PDEV_NEEDS_KEY))
    {
        LOG(ERROR, "PDEV communicate failed \n");
        goto exit;
    }

    LOG(TEST, "\n\tPDEV state has been changed to PDEV_NEEDS_KEY\n");

    /* Get public key */
    rc = val_host_get_public_key_from_cert_chain(pdev_dev->cert_chain,
                         pdev_dev->cert_chain_len,
                         pdev_dev->public_key,
                         &pdev_dev->public_key_len,
                         pdev_dev->public_key_metadata,
                         &pdev_dev->public_key_metadata_len,
                         &public_key_algo);
    if (rc != 0) {
        LOG(ERROR, "Get public key failed\n");
        goto exit;
    }

    if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P256) {
        pdev_dev->public_key_sig_algo = RMI_SIG_ECDSA_P256;
    } else if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P384) {
        pdev_dev->public_key_sig_algo = RMI_SIG_ECDSA_P384;
    } else {
        pdev_dev->public_key_sig_algo = RMI_SIG_RSASSA_3072;
    }

    pubkey_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!pubkey_params)
    {
        LOG(ERROR, "\tFailed to allocate memory for pub_key\n");
        goto exit;
    }

    val_memset(pubkey_params, 0, PAGE_SIZE_4K);
    pubkey_params->key_addr = (uint64_t)pdev_dev->public_key;
    pubkey_params->metadata_addr = (uint64_t)pdev_dev->public_key_metadata;
    pubkey_params->key_len = pdev_dev->public_key_len;
    pubkey_params->metadata_len = pdev_dev->public_key_metadata_len;

    args = val_host_rmi_pdev_set_pubkey(pdev_dev->pdev, (uint64_t)pubkey_params);
    val_host_mem_free(pubkey_params);
    pubkey_params = NULL;
    if (args.x0)
    {
        LOG(ERROR, "Pdev set key failed %lx\n", args.x0, 0);
        goto exit;
    }

    args = val_host_rmi_pdev_get_state(pdev_dev->pdev);
    if (args.x0)
    {
        LOG(ERROR, "Pdev get state failed\n");
        goto exit;
    }

    /* PDEV state should be RMI_PDEV_HAS_KEY */
    if (args.x1 != RMI_PDEV_HAS_KEY)
    {
        LOG(ERROR, "Pdev state should be PDEV_HAS_KEY\n");
        goto exit;
    }

    LOG(TEST, "\n\tPDEV state has been changed to PDEV_HAS_KEY\n\n");

    if (val_host_dev_communicate(NULL, pdev_dev, NULL, RMI_PDEV_READY))
    {
        LOG(ERROR, "PDEV communicate failed \n");
        goto exit;
    }

    LOG(TEST, "\n\tPDEV state has been changed to RMI_PDEV_READY\n");

    return pdev_dev->pdev;
exit:
    if (pubkey_params != NULL)
        val_host_mem_free(pubkey_params);
    return VAL_TEST_PREP_SEQ_FAILED;

}

static bool pdev_stream_pdev2_required(uint8_t stream_type)
{
    return (stream_type != RMI_PDEV_STREAM_NCOH_SYS) &&
           (stream_type != RMI_PDEV_STREAM_COH_SYS);
}

static uint32_t root_pdev_cleanup(val_host_pdev_stream_ts *pdev_stream)
{
    val_host_pdev_ts *rp_pdev = &pdev_stream->rp_pdev_obj;
    uint32_t status = VAL_SUCCESS;
    uint64_t ret;

    if (pdev_stream->rp_pdev_created)
    {
        if (val_host_pdev_teardown(rp_pdev, rp_pdev->pdev))
            status = VAL_ERROR;
        else
            pdev_stream->rp_pdev_created = 0;
    }

    if (pdev_stream->rp_pdev_delegated &&
        !pdev_stream->rp_pdev_created)
    {
        ret = val_host_rmi_granule_range_undelegate(
                                  rp_pdev->pdev,
                                  rp_pdev->pdev + PAGE_SIZE);
        if (ret)
            status = VAL_ERROR;
        else
            pdev_stream->rp_pdev_delegated = 0;
    }

    if (!pdev_stream->rp_pdev_created &&
        !pdev_stream->rp_pdev_delegated)
    {
        if (rp_pdev->dev_comm_data != NULL)
        {
            if (rp_pdev->dev_comm_data->enter.req_addr != 0U)
                val_host_mem_free((void *)rp_pdev->dev_comm_data->enter.req_addr);
            if (rp_pdev->dev_comm_data->enter.rsp_addr != 0U)
                val_host_mem_free((void *)rp_pdev->dev_comm_data->enter.rsp_addr);
            val_host_mem_free(rp_pdev->dev_comm_data);
            rp_pdev->dev_comm_data = NULL;
        }

        if (rp_pdev->pdev != 0U)
        {
            val_host_mem_free((void *)rp_pdev->pdev);
            rp_pdev->pdev = 0;
        }
    }

    return status;
}

uint64_t g_root_pdev_ready_prep_sequence(
                                  val_host_pdev_stream_ts *pdev_stream,
                                  uint32_t rp_bdf)
{
    val_host_rmi_pdev_params_ts *params = NULL;
    val_host_pdev_flags_ts flags;
    val_host_pdev_ts *rp_pdev = &pdev_stream->rp_pdev_obj;
    uint64_t ret;

    rp_pdev->pdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((rp_pdev->pdev == 0U) || (params == NULL))
        goto exit;

    ret = val_host_rmi_granule_range_delegate(rp_pdev->pdev,
                                               rp_pdev->pdev + PAGE_SIZE);
    if (ret)
        goto exit;
    pdev_stream->rp_pdev_delegated = 1;

    val_memset(params, 0, PAGE_SIZE);
    val_memset(&flags, 0, sizeof(flags));
    flags.category = RMI_PDEV_ROOT_PORT;
    val_memcpy(&params->flags, &flags, sizeof(flags));
    params->hb_base = val_pcie_get_ecam_base(rp_bdf);
    params->pdev_id = PCIE_CREATE_BDF_PACKED(rp_bdf);
    params->routing_id = PCIE_EXTRACT_BDF_SEG(rp_bdf);
    params->rid_base = PCIE_CREATE_BDF_PACKED(rp_bdf);
    params->rid_top = params->rid_base + 1;

    ret = val_host_rmi_pdev_create(rp_pdev->pdev, (uint64_t)params);
    val_host_mem_free(params);
    params = NULL;
    if (ret)
        goto exit;
    pdev_stream->rp_pdev_created = 1;

    rp_pdev->bdf = rp_bdf;
    rp_pdev->pdev_flags = flags;
    rp_pdev->dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (rp_pdev->dev_comm_data == NULL)
        goto exit;

    val_memset(rp_pdev->dev_comm_data, 0, PAGE_SIZE);
    rp_pdev->dev_comm_data->enter.req_addr =
        (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (rp_pdev->dev_comm_data->enter.req_addr == 0U)
        goto exit;

    rp_pdev->dev_comm_data->enter.rsp_addr =
        (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (rp_pdev->dev_comm_data->enter.rsp_addr == 0U)
        goto exit;

    if (val_host_dev_communicate(NULL, rp_pdev, NULL, RMI_PDEV_READY))
        goto exit;

    return rp_pdev->pdev;

exit:
    if (params != NULL)
        val_host_mem_free(params);
    (void)root_pdev_cleanup(pdev_stream);

    return VAL_TEST_PREP_SEQ_FAILED;
}

uint32_t g_pdev_stream_communicate_prep_sequence(
                                  val_host_pdev_ts *pdev_dev,
                                  val_host_pdev_stream_ts *pdev_stream)
{
    bool pdev_complete = false;

    if ((pdev_dev == NULL) || (pdev_stream == NULL))
        return VAL_ERROR;

    if (pdev_stream->rp_pdev_obj.pdev != 0U)
        return val_host_pdev_stream_communicate(
                                  pdev_dev,
                                  &pdev_stream->rp_pdev_obj);

    do
    {
        if (val_host_dev_communicate(NULL, pdev_dev, NULL,
                                     RMI_PDEV_READY))
            return VAL_ERROR;
        pdev_complete = val_host_dev_comm_complete(
                            pdev_dev->dev_comm_data->exit.flags);
    } while (!pdev_complete);

    return VAL_SUCCESS;
}

uint64_t g_pdev_stream_connected_prep_sequence(
                                  val_host_pdev_ts *pdev_dev,
                                  val_host_pdev_stream_ts *pdev_stream,
                                  uint8_t stream_type)
{
    val_host_rmi_pdev_stream_params_ts *params = NULL;
    val_host_pdev_flags_ts pdev_flags;
    val_smc_param_ts args;
    uint32_t num_bdf, rp_bdf = 0;
    bool endpoint_found = false;
    bool pdev2_required;
    uint64_t i;

    if ((pdev_dev == NULL) || (pdev_stream == NULL))
        return VAL_TEST_PREP_SEQ_FAILED;

    val_memset(pdev_dev, 0, sizeof(*pdev_dev));
    val_memset(pdev_stream, 0, sizeof(*pdev_stream));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR)
        goto exit;

    pdev2_required = pdev_stream_pdev2_required(stream_type);
    while (num_bdf != 0U)
    {
        if (val_pcie_find_doe_capability(&num_bdf, &pdev_dev->bdf,
                                         &pdev_dev->doe_cap_base))
            break;

        if ((pdev2_required &&
             (val_pcie_device_port_type(pdev_dev->bdf) == EP)) ||
            (!pdev2_required &&
             (val_pcie_device_port_type(pdev_dev->bdf) == RCiEP)))
        {
            endpoint_found = true;
            break;
        }
    }

    if (!endpoint_found)
    {
        if (!pdev2_required)
            return VAL_SKIP_CHECK;

        goto exit;
    }

    if (pdev2_required &&
        val_pcie_get_rootport(pdev_dev->bdf, &rp_bdf))
        goto exit;

    val_host_set_pdev_flags(&pdev_flags);
    if (!pdev2_required)
        pdev_flags.category = RMI_PDEV_ENDPOINT_ACCEL_ON_CHIP;
    pdev_dev->pdev_flags = pdev_flags;
    pdev_dev->root_id = (uint16_t)rp_bdf;
    pdev_dev->pdev = g_pdev_ready_prep_sequence(pdev_dev);
    if (pdev_dev->pdev == VAL_TEST_PREP_SEQ_FAILED)
        goto exit;

    if (pdev2_required &&
        (g_root_pdev_ready_prep_sequence(pdev_stream, rp_bdf) ==
         VAL_TEST_PREP_SEQ_FAILED))
        goto exit;

    if ((pdev_dev->ncoh_num_addr_range > NCOH_ADDR_RANGE_NUM) ||
        (pdev_dev->ncoh_num_addr_range > MAX_PDEV_ADDR))
        goto exit;

    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (params == NULL)
        goto exit;

    val_memset(params, 0, PAGE_SIZE);
    params->stream_type = stream_type;
    params->pdev_1 = pdev_dev->pdev;
    params->pdev_2 = pdev_stream->rp_pdev_obj.pdev;
    params->num_addr_range = pdev_dev->ncoh_num_addr_range;
    for (i = 0; i < params->num_addr_range; i++)
    {
        params->addr_range[i].base = pdev_dev->ncoh_addr_range[i].base;
        params->addr_range[i].top = pdev_dev->ncoh_addr_range[i].top;
    }

    args = val_host_rmi_pdev_stream_connect((uint64_t)params);
    val_host_mem_free(params);
    params = NULL;
    if (args.x0 != RMI_SUCCESS)
        goto exit;
    pdev_stream->stream_handle = args.x1;

    if (g_pdev_stream_communicate_prep_sequence(pdev_dev, pdev_stream))
        goto exit;

    args = val_host_rmi_pdev_stream_complete(
                                  pdev_dev->pdev,
                                  pdev_stream->rp_pdev_obj.pdev,
                                  pdev_stream->stream_handle);
    if (args.x0 != RMI_SUCCESS)
        goto exit;

    return pdev_stream->stream_handle;

exit:
    if (params != NULL)
        val_host_mem_free(params);

    return VAL_TEST_PREP_SEQ_FAILED;
}

uint32_t g_pdev_stream_teardown_sequence(
                                  val_host_pdev_ts *pdev_dev,
                                  val_host_pdev_stream_ts *pdev_stream)
{
    val_smc_param_ts args;
    uint32_t status = VAL_SUCCESS;

    if ((pdev_dev == NULL) || (pdev_stream == NULL))
        return VAL_ERROR;

    if (pdev_stream->stream_handle != 0U)
    {
        args = val_host_rmi_pdev_stream_disconnect(
                                  pdev_dev->pdev,
                                  pdev_stream->rp_pdev_obj.pdev,
                                  pdev_stream->stream_handle);
        if ((args.x0 != RMI_SUCCESS) ||
            g_pdev_stream_communicate_prep_sequence(pdev_dev, pdev_stream))
        {
            status = VAL_ERROR;
        }
        else
        {
            args = val_host_rmi_pdev_stream_complete(
                                  pdev_dev->pdev,
                                  pdev_stream->rp_pdev_obj.pdev,
                                  pdev_stream->stream_handle);
            if (args.x0 != RMI_SUCCESS)
                status = VAL_ERROR;
        }
        if (status == VAL_SUCCESS)
            pdev_stream->stream_handle = 0;
    }

    if ((pdev_stream->stream_handle == 0U) &&
        root_pdev_cleanup(pdev_stream))
        status = VAL_ERROR;

    return status;
}
