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
        ret = val_host_rmi_granule_delegate(realm->rd);
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
        ret = val_host_rmi_granule_delegate(realm->rtt_l0_addr);
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
                    ret = val_host_rmi_granule_delegate(realm->rtt_aux_l0_addr[i]
                                                                     + (j * PAGE_SIZE));
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
    params->rtt_level_start = realm->s2_starting_level;
    params->rtt_num_start = realm->num_s2_sl_rtts;
    params->vmid = realm->vmid;
    params->num_aux_planes = realm->num_aux_planes;
    params->flags = realm->flags;
    params->flags1 = realm->flags1;
    for (i = 0; i < realm->num_aux_planes; i++)
        params->aux_vmid[i] = (uint16_t)(realm->vmid + i + 1);
    for (i = 0; i < realm->num_aux_planes; i++)
        params->aux_rtt_base[i] = realm->rtt_aux_l0_addr[i];
    /* RealmParams structure takes the number of breakpoints, minus one */
    params->num_bps = realm->num_bps + 1;
    params->num_wps = realm->num_wps + 1;
    params->mecid = realm->mecid;

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
    ret = val_host_rmi_granule_undelegate(realm->rtt_l0_addr);
    if (ret)
    {
        LOG(WARN, "rtt undelegation failed, rtt_addr=0x%x, ret=0x%x\n", realm->rtt_l0_addr, ret);
    }

free_rtt:
     val_host_mem_free((void *)realm->rtt_l0_addr);

undelegate_rd:
    ret = val_host_rmi_granule_undelegate(realm->rd);
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

    uint64_t ret, i, mpidr = 0x0, aux_count, j;

    if (realm->rec_count > VAL_MAX_REC_COUNT)
    {
        LOG(ERROR, "max supported recs are VAL_MAX_REC_COUNT\n");
        return VAL_ERROR;
    }

    /* Get aux granules count */
    ret = val_host_rmi_rec_aux_count(realm->rd, &aux_count);
    if (ret)
    {
        LOG(ERROR, "REC AUX count failed, ret=0x%x\n", ret);
        return VAL_ERROR;
    } else {
        if (aux_count > VAL_MAX_REC_AUX_GRANULES)
        {
            LOG(ERROR, "max supported aux granules are VAL_MAX_REC_AUX_GRANULES\n");
            return VAL_ERROR;
        }

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
    rec_params->num_aux = aux_count;
    realm->aux_count = aux_count;

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
            ret = val_host_rmi_granule_delegate(realm->rec[i]);
            if (ret)
            {
                LOG(ERROR, "rec delegation failed, rec=0x%x, ret=0x%x\n", realm->rec[i], ret);
                goto free_rec_params;
            }
        }

        for (j = 0; j < aux_count; j++)
        {
            rec_params->aux[j] = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
            if (!rec_params->aux[j])
            {
                LOG(ERROR, "Failed to allocate memory for aux rec\n");
                goto free_rec_params;
            } else {
                ret = val_host_rmi_granule_delegate(rec_params->aux[j]);
                if (ret)
                {
                    LOG(ERROR, "rec delegation failed, rec=0x%x, ret=0x%x\n",
                                         rec_params->aux[j], ret);
                    goto free_rec_params;
                }
            }
            // realm->aux[j + (i * aux_count)] = rec_params->aux[j];
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
        ret = val_host_rmi_granule_undelegate(realm->rec[i]);
        if (ret)
        {
            LOG(WARN, "rec undelegation failed, rec=0x%x, ret=0x%x\n", realm->rec[i], ret);
        }

        val_host_mem_free((void *)realm->rec[i]);
        val_host_mem_free((void *)realm->run[i]);

        for (j = 0; j < aux_count; j++)
        {
            ret = val_host_rmi_granule_undelegate(realm->rec_aux_granules[j + (i * aux_count)]);
            if (ret)
            {
                LOG(WARN, "granule undelegation failed, PA=0x%x, ret=0x%x\n",
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
    return (uint64_t)val_get_secure_base();
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

    if (val_host_rmi_data_create(rd, data, ipa, src, flags))
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

    if (val_host_rmi_data_create(rd, data, IPA_ADDR_PROTECTED_ASSIGNED_RAM, src, flags))
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

    if (val_host_rmi_data_create_unknown(rd, data, IPA_ADDR_PROTECTED_ASSIGNED_EMPTY))
    {
        LOG(ERROR, "Couldn't complete the assigned protected mapping\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return IPA_ADDR_PROTECTED_ASSIGNED_EMPTY;
}

uint64_t ipa_protected_destroyed_prep_sequence(uint64_t rd1)
{
    val_host_data_destroy_ts data_destroy;

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

    if (val_host_rmi_data_create(rd, data, IPA_ADDR_DESTROYED, src, flags))
    {
        LOG(ERROR, "Couldn't complete the assigned protected mapping\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (val_host_rmi_data_destroy(rd, IPA_ADDR_DESTROYED, &data_destroy))
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

    if (val_host_rmi_rtt_map_unprotected(rd1, IPA_ADDR_UNPROTECTED, MAP_LEVEL, desc))
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


uint64_t g_pdev_new_prep_sequence(val_host_pdev_ts *pdev_dev)
{
    uint64_t i, ret, aux_count;
    val_host_pdev_params_ts *pdev_params;
    val_smc_param_ts args;
    uint64_t req_addr, resp_addr;

    /* Allocate and delegate PDEV */
    pdev_dev->pdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!pdev_dev->pdev)
    {
        LOG(ERROR, "Failed to allocate memory for pdev\n");
        goto exit;
    } else {
        ret = val_host_rmi_granule_delegate(pdev_dev->pdev);
        if (ret)
        {
            LOG(ERROR, "pdev delegation failed, pdev=0x%x, ret=0x%x\n", pdev_dev->pdev, ret);
            goto exit;
        }
    }

    /* Allocate memory for params */
    pdev_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev_params == NULL)
    {
        LOG(ERROR, "Failed to allocate memory for pdev_params\n");
        goto exit;
    }

    val_memset(pdev_params, 0, PAGE_SIZE_4K);

    /* Get aux granules count */
    args = val_host_rmi_pdev_aux_count(pdev_dev->pdev_flags);
    if (args.x0) {
        LOG(ERROR, "PDEV AUX count ABI failed, ret value is: %x\n", args.x0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    aux_count = args.x1;

    /* Populate params */
    pdev_params->flags = pdev_dev->pdev_flags;
    pdev_params->hash_algo = RMI_HASH_SHA_256;
    pdev_params->pdev_id = pdev_dev->bdf;
    pdev_params->cert_id = 0;
    pdev_params->ncoh_ide_sid = 0;
    pdev_params->num_aux = aux_count;
    pdev_params->ecam_addr = PLATFORM_OVERRIDE_PCIE_ECAM_BASE_ADDR_0;
    pdev_params->root_id = pdev_dev->root_id;

    for (i = 0; i < pdev_params->num_aux; i++)
    {
        pdev_params->aux[i] = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
        if (!pdev_params->aux[i])
        {
            LOG(ERROR, "Failed to allocate memory for aux pdev\n");
            goto exit;
        }
        else
        {
            ret = val_host_rmi_granule_delegate(pdev_params->aux[i]);
            if (ret)
            {
                LOG(ERROR, "pdev delegation failed, pdev=0x%x, ret=0x%x", pdev_params->aux[i],
                                                                               ret);
                goto exit;
            }
        }
    }

    /* Create pdev */
    args = val_host_rmi_pdev_create(pdev_dev->pdev, (uint64_t)pdev_params);
    if (args.x0)
    {
        LOG(ERROR, "Pdev create failed\n");
        goto exit;
    }

    args = val_host_rmi_pdev_get_state(pdev_dev->pdev);
    if (args.x0)
    {
        LOG(ERROR, "Pdev get state failed\n");
        goto exit;
    }

    if (args.x1 != RMI_PDEV_NEW)
    {
        LOG(ERROR, "Pdev state should be PDEV_NEW\n");
        goto exit;
    }

    /* Allocate buffer to cache VCA */
    pdev_dev->vca = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_PDEV_VCA_LEN_MAX);
    pdev_dev->vca_len = 0;
    if (pdev_dev->vca == NULL) {
        goto exit;
    }

    /* Allocate memory for req addr buffer */
    req_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!req_addr)
    {
        LOG(ERROR, "Failed to allocate memory for req_addr");
        goto exit;
    }

    /* Allocate memory for resp_addr buffer */
    resp_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!resp_addr)
    {
        LOG(ERROR, "Failed to allocate memory for resp_addr");
        goto exit;
    }

    pdev_dev->dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    pdev_dev->dev_comm_data->enter.req_addr = req_addr;
    pdev_dev->dev_comm_data->enter.resp_addr = resp_addr;

    LOG(DBG, "\nPDEV Create is successful and PDEV state is PDEV_NEW\n");
    return pdev_dev->pdev;
exit:
    return VAL_TEST_PREP_SEQ_FAILED;

}

uint64_t g_vdev_new_prep_sequence(val_host_pdev_ts *pdev_dev,  val_host_vdev_ts *vdev_dev,
                                                                val_host_realm_ts *realm)
{
    uint64_t feature_reg, ret, req_addr, resp_addr, vdev;
    val_host_vdev_params_ts *vdev_params;
    val_smc_param_ts args;
    uint64_t j, flags_pdev, flags_vdev, aux_count;
    val_host_realm_flags_ts realm_flags;
    val_host_public_key_params_ts *pubkey_params;
    val_host_vdev_flags_ts vdev_flags;
    int rc;
    uint8_t public_key_algo;

    /* Read Feature Register 0 and check for DA support */
    val_host_rmi_features(0, &feature_reg);
    if (VAL_EXTRACT_BITS(feature_reg, 42, 42) == 0) {
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
    pdev_dev->dev_comm_data->enter.resp_addr = resp_addr;

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
    val_memcpy(pubkey_params->key, pdev_dev->public_key, pdev_dev->public_key_len);
    val_memcpy(pubkey_params->metadata, pdev_dev->public_key_metadata,
                                pdev_dev->public_key_metadata_len);
    pubkey_params->key_len = pdev_dev->public_key_len;
    pubkey_params->metadata_len = pdev_dev->public_key_metadata_len;
    pubkey_params->algo = pdev_dev->public_key_sig_algo;

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

    /* PDEV state should be RMI_PDEV_NEW */
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

    val_memset(realm, 0, sizeof(realm));
    val_memset(&realm_flags, 0, sizeof(realm_flags));
    /* Overwrite Realm Parameters */
    realm_flags.da = RMI_FEATURE_TRUE;

    val_host_realm_params(realm);
    realm->vmid = vdev_dev->vmid;
    if (vdev_dev->rec_count)
        realm->rec_count = vdev_dev->rec_count;

    val_memcpy(&realm->flags, &realm_flags, sizeof(realm->flags));

    /* Populate realm with one REC*/
    if (val_host_realm_setup(realm, 1))
    {
        LOG(ERROR, "\tRealm setup failed\n");
        goto exit;
    }

    /* Create VDEV */
    /* Allocate and delegate VDEV */
    vdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!vdev)
    {
        LOG(ERROR, "Failed to allocate memory for vdev\n");
        goto exit;
    } else {
        ret = val_host_rmi_granule_delegate(vdev);
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
    val_memset(&vdev_flags, 0, sizeof(vdev_flags));
    vdev_params->flags = 0;
    vdev_params->vdev_id = vdev_dev->vdev_id;
    vdev_params->tdi_id = pdev_dev->bdf;

    val_memcpy(&flags_pdev, &pdev_dev->pdev_flags, sizeof(pdev_dev->pdev_flags));
    val_memcpy(&flags_vdev, &vdev_flags, sizeof(vdev_flags));
    args = val_host_rmi_vdev_aux_count(flags_pdev, flags_vdev);
    if (args.x0)
    {
        LOG(ERROR, "VDEV AUX count ABI failed, ret=%lx\n", args.x0);
        goto exit;
    }

    aux_count = args.x1;
    vdev_params->num_aux = aux_count;

    for (j = 0; j < vdev_params->num_aux; j++)
    {
        vdev_params->aux[j] = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
        if (!vdev_params->aux[j])
        {
            LOG(ERROR, "Failed to allocate memory for aux vdev\n");
            goto exit;
        }
        else
        {
            ret = val_host_rmi_granule_delegate(vdev_params->aux[j]);
            if (ret)
            {
                LOG(ERROR, "vdev delegation failed, vdev=0x%x, ret=0x%x", vdev_params->aux[j],
                                                                               ret);
                goto exit;
            }
        }
    }

    /* Create vdev */
    args = val_host_rmi_vdev_create(realm->rd, pdev_dev->pdev, vdev, (uint64_t)vdev_params);
    if (args.x0)
    {
        LOG(ERROR, "Vdev create failed\n");
        goto exit;
    }

    vdev_dev->vdev = vdev;
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
    vdev_dev->dev_comm_data->enter.resp_addr = resp_addr;
    vdev_dev->dev_comm_data->enter.status = RMI_DEV_COMM_NONE;
    vdev_dev->dev_comm_data->enter.resp_len = 0;
    LOG(TEST, "\n\tVDEV Create is successful and VDEV state is VDEV_NEW\n");

    return vdev;
exit:
    return VAL_TEST_PREP_SEQ_FAILED;

}

uint64_t g_pdev_ready_prep_sequence(val_host_pdev_ts *pdev_dev)
{
    uint64_t feature_reg, req_addr, resp_addr;
    val_smc_param_ts args;
    val_host_public_key_params_ts *pubkey_params;
    int rc;
    uint8_t public_key_algo;

    /* Read Feature Register 0 and check for DA support */
    val_host_rmi_features(0, &feature_reg);
    if (VAL_EXTRACT_BITS(feature_reg, 42, 42) == 0) {
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
    pdev_dev->dev_comm_data->enter.resp_addr = resp_addr;

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
    val_memcpy(pubkey_params->key, pdev_dev->public_key, pdev_dev->public_key_len);
    val_memcpy(pubkey_params->metadata, pdev_dev->public_key_metadata,
                                pdev_dev->public_key_metadata_len);
    pubkey_params->key_len = pdev_dev->public_key_len;
    pubkey_params->metadata_len = pdev_dev->public_key_metadata_len;
    pubkey_params->algo = pdev_dev->public_key_sig_algo;

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

    /* PDEV state should be RMI_PDEV_NEW */
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
    return VAL_TEST_PREP_SEQ_FAILED;

}

void val_host_set_pdev_flags(val_host_pdev_flags_ts *pdev_flags)
{
    pdev_flags->spdm = RMI_SPDM_TRUE;
    pdev_flags->ncoh_ide = RMI_IDE_TRUE;
    pdev_flags->category = RMI_PDEV_SMEM;
}

uint64_t g_vdev_state_prep_sequence(val_host_pdev_ts *pdev_dev,  val_host_vdev_ts *vdev_dev,
                                  val_host_realm_ts *realm, uint8_t vdev_state)
{
    uint64_t feature_reg, ret, req_addr, resp_addr, vdev;
    val_host_vdev_params_ts *vdev_params;
    val_smc_param_ts args;
    uint64_t j, flags_pdev, flags_vdev, aux_count;
    val_host_realm_flags_ts realm_flags;
    val_host_public_key_params_ts *pubkey_params;
    val_host_vdev_flags_ts vdev_flags;
    int rc;
    uint8_t public_key_algo;
    val_host_vdev_measure_params_ts *vdev_measure_params;

    /* Read Feature Register 0 and check for DA support */
    val_host_rmi_features(0, &feature_reg);
    if (VAL_EXTRACT_BITS(feature_reg, 42, 42) == 0) {
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
    pdev_dev->dev_comm_data->enter.resp_addr = resp_addr;

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
    val_memcpy(pubkey_params->key, pdev_dev->public_key, pdev_dev->public_key_len);
    val_memcpy(pubkey_params->metadata, pdev_dev->public_key_metadata,
                                pdev_dev->public_key_metadata_len);
    pubkey_params->key_len = pdev_dev->public_key_len;
    pubkey_params->metadata_len = pdev_dev->public_key_metadata_len;
    pubkey_params->algo = pdev_dev->public_key_sig_algo;

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

    /* PDEV state should be RMI_PDEV_NEW */
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

    val_memset(realm, 0, sizeof(*realm));
    val_memset(&realm_flags, 0, sizeof(realm_flags));
    /* Overwrite Realm Parameters */
    realm_flags.da = RMI_FEATURE_TRUE;

    val_host_realm_params(realm);
    realm->vmid = vdev_dev->vmid;
    if (vdev_dev->rec_count)
        realm->rec_count = vdev_dev->rec_count;

    val_memcpy(&realm->flags, &realm_flags, sizeof(realm->flags));

    /* Populate realm with one REC*/
    if (val_host_realm_setup(realm, 1))
    {
        LOG(ERROR, "\tRealm setup failed\n");
        goto exit;
    }

    /* Create VDEV */
    /* Allocate and delegate VDEV */
    vdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!vdev)
    {
        LOG(ERROR, "Failed to allocate memory for vdev\n");
        goto exit;
    } else {
        ret = val_host_rmi_granule_delegate(vdev);
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
    val_memset(&vdev_flags, 0, sizeof(vdev_flags));
    vdev_params->flags = 0;
    vdev_params->vdev_id = vdev_dev->vdev_id;
    vdev_params->tdi_id = pdev_dev->bdf;

    val_memcpy(&flags_pdev, &pdev_dev->pdev_flags, sizeof(pdev_dev->pdev_flags));
    val_memcpy(&flags_vdev, &vdev_flags, sizeof(vdev_flags));
    args = val_host_rmi_vdev_aux_count(flags_pdev, flags_vdev);
    if (args.x0)
    {
        LOG(ERROR, "VDEV AUX count ABI failed, ret=%lx\n", args.x0);
        goto exit;
    }

    aux_count = args.x1;
    vdev_params->num_aux = aux_count;

    for (j = 0; j < vdev_params->num_aux; j++)
    {
        vdev_params->aux[j] = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
        if (!vdev_params->aux[j])
        {
            LOG(ERROR, "Failed to allocate memory for aux vdev\n");
            goto exit;
        }
        else
        {
            ret = val_host_rmi_granule_delegate(vdev_params->aux[j]);
            if (ret)
            {
                LOG(ERROR, "vdev delegation failed, vdev=0x%x, ret=0x%x", vdev_params->aux[j],
                                                                               ret);
                goto exit;
            }
        }
    }

    /* Create vdev */
    args = val_host_rmi_vdev_create(realm->rd, pdev_dev->pdev, vdev, (uint64_t)vdev_params);
    if (args.x0)
    {
        LOG(ERROR, "Vdev create failed\n");
        goto exit;
    }

    vdev_dev->vdev = vdev;
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
    vdev_dev->dev_comm_data->enter.resp_addr = resp_addr;

    LOG(TEST, "\n\tVDEV Create is successful and VDEV state is VDEV_NEW\n");

    vdev_dev->pdev = pdev_dev->pdev;
    vdev_dev->vdev = vdev;
    vdev_dev->dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    vdev_dev->dev_comm_data->enter.req_addr = req_addr;
    vdev_dev->dev_comm_data->enter.resp_addr = resp_addr;

   if (vdev_state == RMI_VDEV_NEW)
        return vdev;

    if (val_host_dev_communicate(realm, pdev_dev, vdev_dev, RMI_VDEV_UNLOCKED))
    {
        LOG(ERROR, "VDEV communicate failed.\n");
        goto exit;
    }

    LOG(TEST, "\n\tVDEV state is RMI_VDEV_UNLOCKED\n");
    if (vdev_state == RMI_VDEV_UNLOCKED)
        return vdev;

    /* Allocate buffer to cache device measurements */
    vdev_dev->meas = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_VDEV_MEAS_LEN_MAX);
    if (vdev_dev->meas == NULL) {
        goto exit;
    }

    vdev_dev->meas_len = 0;

    /* Allocate buffer to cache device interface report */
    vdev_dev->ifc_report = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_VDEV_IFC_REPORT_LEN_MAX);
    vdev_dev->ifc_report_len = 0;
    if (vdev_dev->ifc_report == NULL) {
        goto exit;
    }

    /* Allocate memory for vdev params */
    vdev_measure_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (vdev_measure_params == NULL)
    {
        LOG(ERROR, "Failed to allocate memory for vdev_measure_params");
        goto exit;
    }

    val_memset(vdev_measure_params, 0, PAGE_SIZE);

    /*
     * vdev_measure_params->flags is left as 0 as we don't want signed nor
     * raw output.
     */
    /*
     * Set indices value  to (1 << 254) to retrieve all measurements
     * supported by the device.
     */
    vdev_measure_params->indices[31] = (unsigned char)1U << 6U;

    args = val_host_rmi_vdev_get_measurements(realm->rd, vdev_dev->pdev, vdev_dev->vdev,
                        (uint64_t)vdev_measure_params);
    if (args.x0)
    {
        LOG(ERROR, "\tVDEV_GET_MEASUREMENTS failed, ret=%x\n", args.x0);
        goto exit;
    }

    if (val_host_dev_communicate(realm, pdev_dev, vdev_dev, RMI_VDEV_UNLOCKED))
    {
        LOG(ERROR, "VDEV communicate failed.\n");
        goto exit;
    }

    args = val_host_rmi_vdev_lock(realm->rd, vdev_dev->pdev, vdev_dev->vdev);
    if (args.x0)
    {
        LOG(ERROR, "\tVDEV_LOCK failed, ret=%x\n", args.x0);
        goto exit;
    }

    if (val_host_dev_communicate(realm, pdev_dev, vdev_dev, RMI_VDEV_LOCKED))
    {
        LOG(ERROR, "VDEV communicate failed.\n");
        goto exit;
    }

    args = val_host_rmi_vdev_get_interface_report(realm->rd, vdev_dev->pdev,
                                                           vdev_dev->vdev);
    if (args.x0)
    {
        LOG(ERROR, "VDEV_GET_INTERFACE_REPORT failed, ret=%x\n", args.x0);
        goto exit;
    }

    if (val_host_dev_communicate(realm, pdev_dev, vdev_dev, RMI_VDEV_LOCKED))
    {
        LOG(ERROR, "VDEV communicate failed.\n");
        goto exit;
    }

    LOG(TEST, "\n\tVDEV state is RMI_VDEV_LOCKED\n");
    if (vdev_state == RMI_VDEV_LOCKED)
        return vdev;

    args = val_host_rmi_vdev_start(realm->rd, vdev_dev->pdev, vdev_dev->vdev);
    if (args.x0)
    {
        LOG(ERROR, "\tVDEV_START failed, ret=%x\n", args.x0);
        goto exit;
    }

    if (val_host_dev_communicate(realm, pdev_dev, vdev_dev, RMI_VDEV_STARTED))
    {
        LOG(ERROR, "VDEV communicate failed.\n");
        goto exit;
    }
    LOG(TEST, "\n\tVDEV state is RMI_VDEV_STARTED\n");

    return vdev;
exit:
    return VAL_TEST_PREP_SEQ_FAILED;

}
