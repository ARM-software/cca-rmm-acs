/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_rtt_destroy_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512 * L3_SIZE)
#define L1_SIZE (512 * L2_SIZE)
#define L0_SIZE (512UL * L1_SIZE)

#define NUM_REALMS 1
#define VALID_REALM 0

#define IPA_LIVE_RTT 0
#define IPA_ADDR_DATA (5 * L3_SIZE)
#define IPA_NON_LIVE_RTT L2_SIZE
#define IPA_NO_CHILD_RTT (2 * L2_SIZE)
#define IPA_FOLDED_RTT (3 * L2_SIZE)
#define IPA_DESTROYED_RTT (4 * L2_SIZE)
#define MAP_LEVEL 3
#define START_RTT_LEVEL 0
#define MAX_RTT_LEVEL 3
#define WALK_ERR_LEVEL 3

static val_host_realm_ts realm[NUM_REALMS];

static struct argument_store {
    uint64_t rd_valid;
    uint64_t ipa_valid;
    uint64_t level_valid;
} c_args;

struct arguments {
    uint64_t rd;
    uint64_t ipa;
    uint64_t level;
};

static struct cmd_output {
    uint64_t rtt;
    uint64_t top;
} c_exp_output;

static uint64_t ipa_folded_rtt_prep_sequence(void)
{
    val_data_create_ts data_create;
    uint64_t ret, phys, rtt;

    if (create_mapping(IPA_FOLDED_RTT, true, realm[VALID_REALM].rd))
    {
        LOG(ERROR, "\t Mapping creation failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    data_create.size = L2_SIZE;
    phys = (uint64_t)val_host_mem_alloc(L2_SIZE, L2_SIZE);
    if (!phys)
    {
        LOG(ERROR, "\tval_host_mem_alloc failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    data_create.src_pa = phys;
    data_create.target_pa = phys + data_create.size;
    data_create.ipa = IPA_FOLDED_RTT;
    data_create.rtt_alignment = L2_SIZE;
    ret = val_host_map_protected_data_to_realm(&realm[VALID_REALM], &data_create);
    if (ret)
    {
        LOG(ERROR, "\tval_host_map_protected_data_to_realm failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (val_host_rmi_rtt_fold(realm[VALID_REALM].rd, IPA_FOLDED_RTT, MAP_LEVEL, &rtt))
    {
        LOG(ERROR, "\t RTT Fold failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;

    }

    return IPA_FOLDED_RTT;
}

static uint64_t ipa_live_rtt_prep_sequence(void)
{
    return IPA_LIVE_RTT;
}

static uint64_t ipa_no_parent_rtt_prep_sequence(void)
{
    return L1_SIZE;
}

static uint64_t ipa_no_child_rtt_prep_sequence(void)
{
    return IPA_NO_CHILD_RTT;
}

static uint64_t level_invalid_start_prep_sequence(void)
{
    return START_RTT_LEVEL;
}

static uint64_t level_invalid_oob_prep_sequence(void)
{
    return MAX_RTT_LEVEL + 1;
}

static uint64_t g_rec_ready_prep_sequence(uint64_t vmid)
{
    val_host_rec_params_ts rec_params;
    val_memset(&rec_params, 0, sizeof(rec_params));

    realm[vmid].rec_count = 1;

    rec_params.pc = 0;
    rec_params.flags = RMI_RUNNABLE;

    if (val_host_rec_create_common(&realm[vmid], &rec_params))
    {
        LOG(ERROR, "\tREC Create Failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm[vmid].rec[0];
}

static uint64_t g_rd_new_prep_sequence(uint16_t vmid)
{
    val_memset(&realm[vmid], 0, sizeof(realm[vmid]));

    realm[vmid].s2sz = 40;
    realm[vmid].hash_algo = RMI_HASH_SHA_256;
    realm[vmid].s2_starting_level = 0;
    realm[vmid].num_s2_sl_rtts = 1;
    realm[vmid].vmid = vmid;

    if (val_host_realm_create_common(&realm[vmid]))
    {
        LOG(ERROR, "\tRealm create failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm[vmid].rd;
}

/* RMI_RTT_DESTROY expects a non live RTT i.e all the entries are either UNASSINGED
 * or DESTROYED */
static uint64_t ipa_valid_prep_sequence(void)
{
    if (create_mapping(IPA_NON_LIVE_RTT, false, realm[VALID_REALM].rd))
    {
        LOG(ERROR, "\tUnassigned unprotected ipa mapping creation failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return IPA_NON_LIVE_RTT;
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

    c_args.level_valid = MAP_LEVEL;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_OUTSIDE_OF_PERMITTED_PA:
            args->rd = g_outside_of_permitted_pa_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_DEV_MEM:
            args->rd = g_dev_mem_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_STATE_DELEGATED:
            args->rd = g_delegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_STATE_REC:
            args->rd = g_rec_ready_prep_sequence(VALID_REALM);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_STATE_RTT:
            args->rd = realm[VALID_REALM].rtt_l0_addr;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_STATE_DATA:
            args->rd = g_data_prep_sequence(realm[VALID_REALM].rd, IPA_ADDR_DATA);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case LEVEL_INVALID:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = level_invalid_oob_prep_sequence();
            break;

        case LEVEL_STARTING_LEVEL:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = level_invalid_start_prep_sequence();
            break;

        case IPA_LEVEL_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_protected_unassigned_empty_prep_sequence(c_args.rd_valid);
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->level = c_args.level_valid;
            break;

        case IPA_OUT_OF_PERMITTED_IPA:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_outside_of_permitted_ipa_prep_sequence();
            args->level = c_args.level_valid;
            break;

        case IPA_NO_PARENT_RTTE:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_no_parent_rtt_prep_sequence();
            args->level = c_args.level_valid;
            c_exp_output.top = L0_SIZE;
            break;

        case RTTE_STATE_UNASSIGNED:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_no_child_rtt_prep_sequence();
            args->level = c_args.level_valid;
            c_exp_output.top = L1_SIZE;
            break;

        case RTTE_STATE_ASSIGNED:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_folded_rtt_prep_sequence();
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->level = c_args.level_valid;
            c_exp_output.top = L1_SIZE;
            break;

        case RTT_LIVE:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_live_rtt_prep_sequence();
            args->level = c_args.level_valid;
            c_exp_output.top = IPA_NON_LIVE_RTT;
            break;

        default:
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_rtt_destroy_host(void)
{
    uint64_t ret = 0, i;
    struct arguments args;
    val_host_rtt_entry_ts rtte;
    val_host_rtt_destroy_ts out_val;

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "\n\tCheck %d : ", i + 1, 0);
        LOG(TEST, test_data[i].msg, 0, 0);
        LOG(TEST, "; intent id : 0x%x \n", test_data[i].label, 0);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        ret = val_host_rmi_rtt_destroy(args.rd, args.ipa, args.level, &out_val);
        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }

        /* Upon RMI_ERROR_RTT check for top == walk_top */
        else if (ret == RMI_ERROR_RTT && out_val.top != c_exp_output.top) {
            LOG(ERROR, " \tUnexpected command output received. top value: 0x%x ", out_val.top, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }

    }

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                                         c_args.level_valid - 1, &rtte);
    if (ret) {
        LOG(ERROR, "\n\tRead entry failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    c_exp_output.rtt = OA(rtte.desc);

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);

    ret = val_host_rmi_rtt_destroy(c_args.rd_valid, c_args.ipa_valid, c_args.level_valid,
                                                                             &out_val);
    if (ret != 0)
    {
        LOG(ERROR, "\n\t RTT_DESTROY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    if (out_val.rtt != c_exp_output.rtt || out_val.top != IPA_FOLDED_RTT) {
        LOG(ERROR, "\n\tUnexpected Output Values. rtt = 0x%x, top = 0x%x\n",
                                                             out_val.rtt, out_val.top);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }


    /* Check on successful command execution, HIPAS = UNASSINGNED & RIPAS == DESTROYED */
    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid, MAP_LEVEL - 1, &rtte);
    if (ret) {
        LOG(ERROR, "\n\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    if (rtte.state != RMI_UNASSIGNED || rtte.ripas != RMI_DESTROYED)
    {
        LOG(ERROR, "\tUnexpected HIPAS and RIPAS values received. HIPAS: %d, RIPAS: %d ",
                                                                     rtte.state, rtte.ripas);
        LOG(ERROR, "\tExpected. HIPAS: %d, RIPAS: %d ", RMI_UNASSIGNED, RMI_DESTROYED);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    /* Check for granule(rtt).state has changed to DELEGATED by excecuting RTT Creaate */
    ret = val_host_rmi_rtt_create(c_args.rd_valid, out_val.rtt, c_args.ipa_valid,
                                                                         c_args.level_valid);
    if (ret)
    {
        LOG(ERROR, "\n\t RTT_CREATE failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

exit:
    /* Unfold folded RTT to meet postamble requirements */
    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, IPA_FOLDED_RTT,
                                                         VAL_RTT_MAX_LEVEL, &rtte);

    if (ret) {
        LOG(ERROR, "\n\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        return;
    }

    if (rtte.walk_level != realm[VALID_REALM].s2_starting_level) {
        /* Unfold folded RTTs to satisfy postamble requirements */
        ret = val_host_create_rtt_levels(&realm[VALID_REALM], IPA_FOLDED_RTT,
                                    (uint32_t)rtte.walk_level, VAL_RTT_MAX_LEVEL,
                                    L2_SIZE);
        if (ret)
        {
            LOG(ERROR, "\tval_host_create_rtt_level failed\n", 0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
            return;
        }
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    return;
}
