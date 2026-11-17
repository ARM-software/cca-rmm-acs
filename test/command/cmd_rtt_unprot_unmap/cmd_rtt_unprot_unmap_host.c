/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_sro.h"
#include "rmi_rtt_unprot_unmap_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define NUM_REALMS 1
#define VALID_REALM 0
#define IPA_ADDR_DATA  5 * PAGE_SIZE
#define IPA_ADDR_UNPROTECTED (1UL << (IPA_WIDTH - 1))
#define MAP_LEVEL 3
#define L2_SIZE (512UL * PAGE_SIZE)

#define RMI_ADDR_RANGE_DESC_4K_ADDR_MASK VAL_HOST_RMI_ADDR_DESC_ADDR_MASK

static val_host_realm_ts realm[NUM_REALMS];
static uint64_t output_list[PAGE_SIZE / sizeof(uint64_t)]
                            __attribute__((aligned(PAGE_SIZE)));

static struct argument_store {
    uint64_t rd_valid;
    uint64_t ipa_valid;
    uint64_t flags_valid;
} c_args;

struct arguments {
    uint64_t rd;
    uint64_t base;
    uint64_t top;
    uint64_t flags;
    uint64_t oaddr;
};

static uint64_t g_rec_ready_prep_sequence(uint64_t vmid)
{
    val_host_rec_params_ts rec_params;
    val_memset(&rec_params, 0, sizeof(rec_params));

    realm[vmid].rec_count = 1;

    rec_params.pc = 0;
    rec_params.flags = RMI_RUNNABLE;

    if (val_host_rec_create_common(&realm[vmid], &rec_params))
    {
        LOG(ERROR, "REC Create Failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm[vmid].rec[0];
}

static uint64_t g_rd_new_prep_sequence(uint16_t vmid)
{
    if (vmid >= NUM_REALMS)
    {
        LOG(ERROR, "Invalid VMID %u\n", vmid);
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    val_memset(&realm[vmid], 0, sizeof(realm[vmid]));

    realm[vmid].s2sz = 40;
    realm[vmid].hash_algo = RMI_HASH_SHA_256;
    realm[vmid].s2_starting_level = 0;
    realm[vmid].num_s2_sl_rtts = 1;
    realm[vmid].vmid = vmid;

    if (val_host_realm_create_common(&realm[vmid]))
    {
        LOG(ERROR, "Realm create failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm[vmid].rd;
}

static uint64_t ipa_valid_prep_sequence(void)
{
    return ipa_unprotected_assinged_prep_sequence(realm[VALID_REALM].rd);
}

static uint64_t rd_valid_prep_sequence(void)
{
    return g_rd_new_prep_sequence(VALID_REALM);
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.rd_valid = rd_valid_prep_sequence();
    if (c_args.rd_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.ipa_valid = ipa_valid_prep_sequence();
    if (c_args.ipa_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    val_host_rmi_rtt_unmap_flags_ts unmap_flags = {0};

    unmap_flags.oaddr_type = RMI_ADDR_TYPE_SINGLE;
    val_memcpy(&c_args.flags_valid, &unmap_flags, sizeof(unmap_flags));

    return VAL_SUCCESS;
}

static uint64_t mapped_l2_prep_sequence(void)
{
    static uint64_t mapped_base;
    val_host_rmi_rtt_unprot_map_flags_ts map_flags = {0};
    uint64_t base;
    uint64_t pa;
    uint64_t flags = 0;

    if (mapped_base != 0UL)
        return mapped_base;

    base = ADDR_ALIGN_DOWN(ipa_unprotected_unmapped_prep_sequence(), L2_SIZE);
    pa = (uint64_t)val_host_mem_alloc(L2_SIZE, L2_SIZE);
    if (pa == 0UL)
        return VAL_TEST_PREP_SEQ_FAILED;

    map_flags.oaddr_type = RMI_ADDR_TYPE_SINGLE;
    map_flags.s2ap = 3;
    val_memcpy(&flags, &map_flags, sizeof(map_flags));

    if (val_host_rmi_rtt_unprot_map(c_args.rd_valid, base, base + L2_SIZE,
                                    flags, val_host_rmi_addr_range_desc(
                                        pa, 1, RMI_BLOCK_L2,
                                        RMI_OP_MEM_UNDELEGATED)) != RMI_SUCCESS)
        return VAL_TEST_PREP_SEQ_FAILED;

    mapped_base = base;
    return mapped_base;
}

static uint64_t remap_single(uint64_t mapped_pa)
{
    val_host_rmi_rtt_unprot_map_flags_ts map_flags = {0};
    val_smc_param_ts cmd;
    uint64_t map_flags_value = 0;
    uint64_t ret;

    map_flags.oaddr_type = RMI_ADDR_TYPE_SINGLE;
    map_flags.s2ap = 3;
    val_memcpy(&map_flags_value, &map_flags, sizeof(map_flags_value));

    cmd = val_host_rmi_rtt_unprot_map_smc(c_args.rd_valid, c_args.ipa_valid,
                                           c_args.ipa_valid + PAGE_SIZE,
                                           map_flags_value, val_host_rmi_addr_range_desc(
                                               mapped_pa, 1, RMI_PAGE_L3,
                                               RMI_OP_MEM_UNDELEGATED));
    ret = val_host_sro_complete(&cmd);
    if (ret != RMI_SUCCESS || cmd.x1 != c_args.ipa_valid + PAGE_SIZE)
        return VAL_ERROR;

    return VAL_SUCCESS;
}

static uint64_t check_list_positive(uint64_t mapped_pa)
{
    val_host_rmi_rtt_unmap_flags_ts unmap_flags = {0};
    val_host_rtt_entry_ts rtte;
    val_smc_param_ts cmd;
    uint64_t unmap_flags_value = 0;
    uint64_t ret;

    if (remap_single(mapped_pa))
        return VAL_ERROR;

    val_memset(output_list, 0, PAGE_SIZE);
    unmap_flags.oaddr_type = RMI_ADDR_TYPE_LIST;
    unmap_flags.list_count = 1;
    val_memcpy(&unmap_flags_value, &unmap_flags, sizeof(unmap_flags_value));

    cmd = val_host_rmi_rtt_unprot_unmap_smc(c_args.rd_valid, c_args.ipa_valid,
                                             c_args.ipa_valid + PAGE_SIZE,
                                             unmap_flags_value,
                                             (uint64_t)output_list);
    ret = val_host_sro_complete(&cmd);
    if (ret != RMI_SUCCESS || cmd.x1 != c_args.ipa_valid + PAGE_SIZE ||
        cmd.x2 != 0UL || cmd.x3 != 1UL ||
        output_list[0] != val_host_rmi_addr_range_desc(
            mapped_pa, 1, RMI_PAGE_L3, RMI_OP_MEM_UNDELEGATED) ||
        val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                     MAP_LEVEL, &rtte) != RMI_SUCCESS ||
        rtte.state != RMI_UNASSIGNED)
        return VAL_ERROR;

    return VAL_SUCCESS;
}

static uint64_t check_none_positive(uint64_t mapped_pa)
{
    val_host_rtt_entry_ts rtte;
    val_smc_param_ts cmd;
    uint64_t ret;

    if (remap_single(mapped_pa))
        return VAL_ERROR;

    cmd = val_host_rmi_rtt_unprot_unmap_smc(c_args.rd_valid, c_args.ipa_valid,
                                             c_args.ipa_valid + PAGE_SIZE,
                                             RMI_ADDR_TYPE_NONE, 0);
    ret = val_host_sro_complete(&cmd);
    if (ret != RMI_SUCCESS || cmd.x1 != c_args.ipa_valid + PAGE_SIZE ||
        cmd.x2 != 0UL || cmd.x3 != 0UL ||
        val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                     MAP_LEVEL, &rtte) != RMI_SUCCESS ||
        rtte.state != RMI_UNASSIGNED)
        return VAL_ERROR;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    args->rd = c_args.rd_valid;
    args->base = c_args.ipa_valid;
    args->top = c_args.ipa_valid + PAGE_SIZE;
    args->flags = c_args.flags_valid;
    args->oaddr = 0;

    switch (label)
    {
        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            break;


        case RD_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case RD_STATE_DELEGATED:
            args->rd = g_delegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case RD_STATE_REC:
            args->rd = g_rec_ready_prep_sequence(VALID_REALM);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case RD_STATE_RTT:
            args->rd = realm[VALID_REALM].rtt_l0_addr;
            break;

        case RD_STATE_DATA:
            args->rd = g_data_prep_sequence(realm[VALID_REALM].rd, IPA_ADDR_DATA);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case BASE_UNALIGNED:
            args->base = g_unaligned_prep_sequence(c_args.ipa_valid);
            break;

        case TOP_UNALIGNED:
            args->top = g_unaligned_prep_sequence(c_args.ipa_valid + PAGE_SIZE);
            break;

        case SIZE_INVALID:
            args->top = args->base;
            break;

        case IPA_PROTECTED:
            args->base = ipa_protected_unassigned_empty_prep_sequence(
                                                        realm[VALID_REALM].rd);
            if (args->base == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->top = args->base + PAGE_SIZE;
            break;

        case IPA_OOB:
            args->base = ipa_outside_of_permitted_ipa_prep_sequence();
            args->top = args->base + PAGE_SIZE;
            break;

        case OADDR_LIST_UNALIGNED:
            args->flags = RMI_ADDR_TYPE_LIST;
            args->oaddr = 1;
            break;

        case OADDR_LIST_INVALID_PAS:
            args->flags = RMI_ADDR_TYPE_LIST;
            args->oaddr = g_outside_of_permitted_pa_prep_sequence();
            break;

        case RTTE_ALIGN:
            args->base = mapped_l2_prep_sequence();
            if (args->base == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->base += PAGE_SIZE;
            args->top = args->base + PAGE_SIZE;
            break;

        case RTTE_SIZE:
            args->base = ADDR_ALIGN_DOWN(mapped_l2_prep_sequence(), L2_SIZE);
            if (args->base == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->top = args->base + PAGE_SIZE;
            break;

        case RTTE_STATE_UNASSIGNED_NS:
            args->base = ipa_unprotected_unassigned_prep_sequence(
                                                        realm[VALID_REALM].rd);
            if (args->base == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->top = args->base + PAGE_SIZE;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_rtt_unprot_unmap_host(void)
{
    uint64_t ret = 0, i, mapped_pa;
    val_smc_param_ts cmd_ret;
    struct arguments args;
    val_host_rtt_entry_ts rtte;

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
              i + 1, test_data[i].msg, test_data[i].label);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        cmd_ret = val_host_rmi_rtt_unprot_unmap_smc(args.rd, args.base,
                          args.top, args.flags, args.oaddr);
        ret = val_host_sro_complete(&cmd_ret);
        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "Test Failure!The ABI call returned: %xExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                      MAP_LEVEL, &rtte);
    if (ret || (rtte.state != RMI_ASSIGNED)) {
        LOG(ERROR, "Unexpected RTTE before unmap, ret: 0x%x state: %d\n",
                   ret, rtte.state);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }
    mapped_pa = rtte.desc & RMI_ADDR_RANGE_DESC_4K_ADDR_MASK;

    cmd_ret = val_host_rmi_rtt_unprot_unmap_smc(c_args.rd_valid,
                          c_args.ipa_valid, c_args.ipa_valid + PAGE_SIZE,
                          c_args.flags_valid, 0);
    ret = val_host_sro_complete(&cmd_ret);
    if (ret != 0)
    {
        LOG(ERROR, "RTT_UNPROT_UNMAP failed with ret value: %d\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    if (cmd_ret.x1 != (c_args.ipa_valid + PAGE_SIZE)) {
        LOG(ERROR, "Unexpected output top: 0x%x\n", cmd_ret.x1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    if ((cmd_ret.x2 != val_host_rmi_addr_range_desc(
             mapped_pa, 1, RMI_PAGE_L3, RMI_OP_MEM_UNDELEGATED)) ||
        (cmd_ret.x3 != 0)) {
        LOG(ERROR, "Unexpected output range: 0x%lx count: 0x%lx\n",
            cmd_ret.x2, cmd_ret.x3);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                      MAP_LEVEL, &rtte);
    if (ret) {
        LOG(ERROR, "READ_ENTRY failed with ret value: %d\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    if ((rtte.state != RMI_UNASSIGNED) || (rtte.ripas != RMI_EMPTY))
    {
        LOG(ERROR, "Unexpected RTTE, state: %d ripas: %d\n",
                   rtte.state, rtte.ripas);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

    val_host_update_destroy_granule_state(c_args.rd_valid, c_args.ipa_valid,
                                           0, 0, GRANULE_UNPROTECTED,
                                           GRANULE_UNPROTECTED, 0);

    LOG(TEST, "Check %2d : LIST output success\n", ++i);
    if (check_list_positive(mapped_pa)) {
        LOG(ERROR, "RMI_RTT_UNPROT_UNMAP LIST success check failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto exit;
    }

    LOG(TEST, "Check %2d : NONE output success\n", ++i);
    if (check_none_positive(mapped_pa)) {
        LOG(ERROR, "RMI_RTT_UNPROT_UNMAP NONE success check failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
