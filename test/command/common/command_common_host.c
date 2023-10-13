 /*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_rmi.h"
#include "command_common_host.h"
#include "val_host_realm.h"
#include "val_host_command.h"

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

uint32_t val_host_realm_create_common(val_host_realm_ts *realm)
{
    val_host_realm_params_ts *params;
    uint64_t ret;

    /* Allocate and delegate RD */
    realm->rd = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!realm->rd)
    {
        LOG(ERROR, "\tFailed to allocate memory for rd\n", 0, 0);
    } else {
        ret = val_host_rmi_granule_delegate(realm->rd);
        if (ret)
        {
            LOG(ERROR, "\trd delegation failed, rd=0x%x, ret=0x%x\n", realm->rd, ret);
            goto free_rd;
        }
    }

    /* Allocate and delegate RTT */
    realm->rtt_l0_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!realm->rtt_l0_addr)
    {
        LOG(ERROR, "\tFailed to allocate memory for rtt_addr\n", 0, 0);
        goto undelegate_rd;
    } else {
        ret = val_host_rmi_granule_delegate(realm->rtt_l0_addr);
        if (ret)
        {
            LOG(ERROR, "\trtt delegation failed, rtt_addr=0x%x, ret=0x%x\n",
                realm->rtt_l0_addr, ret);
            goto free_rtt;
        }
    }

    /* Allocate memory for params */
    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (params == NULL)
    {
        LOG(ERROR, "\tFailed to allocate memory for params\n", 0, 0);
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

    /* Create realm */
    if (val_host_rmi_realm_create(realm->rd, (uint64_t)params))
    {
        LOG(ERROR, "\tRealm create failed\n", 0, 0);
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

    return VAL_ERROR;
}

uint32_t val_host_rec_create_common(val_host_realm_ts *realm, val_host_rec_params_ts *params)
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
            // realm->aux[j + (i * aux_count)] = rec_params->aux[j];
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
        LOG(ERROR, "\tCouldn't create the assigned protected mapping\n", 0, 0);
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
        LOG(ERROR, "\tCouldn't complete the assigned protected mapping\n", 0, 0);
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
        LOG(ERROR, "\tCouldn't create the assigned protected mapping\n", 0, 0);
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
        LOG(ERROR, "\tCouldn't complete the assigned protected mapping\n", 0, 0);
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
        LOG(ERROR, "\tCouldn't create the assigned protected mapping\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    data = g_delegated_prep_sequence();
    if (data == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    if (val_host_rmi_data_create_unknown(rd, data, IPA_ADDR_PROTECTED_ASSIGNED_EMPTY))
    {
        LOG(ERROR, "\tCouldn't complete the assigned protected mapping\n", 0, 0);
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
        LOG(ERROR, "\tCouldn't create the assigned protected mapping\n", 0, 0);
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
        LOG(ERROR, "\tCouldn't complete the assigned protected mapping\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (val_host_rmi_data_destroy(rd, IPA_ADDR_DESTROYED, &data_destroy))
    {
        LOG(ERROR, "\tCouldn't complete the destroyed mapping\n", 0, 0);
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
        LOG(ERROR, "\tCouldn't create the unprotected mapping\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    uint64_t ns = g_undelegated_prep_sequence();
    if (ns == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    uint64_t desc = (ns | ATTR_NORMAL_WB_WA_RA | ATTR_STAGE2_AP_RW | ATTR_INNER_SHARED);

    if (val_host_rmi_rtt_map_unprotected(rd1, IPA_ADDR_UNPROTECTED, MAP_LEVEL, desc))
    {
        LOG(ERROR, "\tCouldn't complete the unprotected mapping\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    return IPA_ADDR_UNPROTECTED;
}

uint64_t ipa_unprotected_unassigned_prep_sequence(uint64_t rd1)
{
    /* Pick an address that is within unprotected range and has rtte.state = UNASSIGNED */
    if (create_mapping(IPA_ADDR_UNPROTECTED_UNASSIGNED, false, rd1))
    {
        LOG(ERROR, "\tUnassigned unprotected ipa mapping creation failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    return IPA_ADDR_UNPROTECTED_UNASSIGNED;
}

uint64_t ipa_protected_unassigned_ram_prep_sequence(uint64_t rd)
{
    if (create_mapping(IPA_ADDR_PROTECTED_UNASSIGNED_RAM, true, rd))
    {
        LOG(ERROR, "\tProtected Unassinged ipa mapping creation failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    return IPA_ADDR_PROTECTED_UNASSIGNED_RAM;
}

uint64_t ipa_protected_unassigned_empty_prep_sequence(uint64_t rd)
{
    if (create_mapping(IPA_ADDR_PROTECTED_UNASSIGNED_EMPTY, false, rd))
    {
        LOG(ERROR, "\tProtected Unassinged ipa mapping creation failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    return IPA_ADDR_PROTECTED_UNASSIGNED_EMPTY;
}
