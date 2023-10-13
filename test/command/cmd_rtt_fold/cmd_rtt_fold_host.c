/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_rtt_fold_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512 * L3_SIZE)
#define L1_SIZE (512 * L2_SIZE)
#define L0_SIZE (512UL * L1_SIZE)

#define IPA_HOMOGENEOUS_RTT 0
#define IPA_ADDR_DATA L2_SIZE
#define IPA_FOLDED_RTT (5 * L2_SIZE)
#define IPA_UNMAPPED L0_SIZE

#define MAP_LEVEL 3
#define L_STARTING_LEVEL 0
#define L_1 1
#define L_2 2
#define L_OUT_OF_BOUND 4

#define NUM_REALMS 1
#define VALID_REALM 0

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
} c_exp_output;

static uint64_t ipa_folded_rtt_prep_sequence(void)
{
    uint64_t ret, phys, rtt;
    val_data_create_ts data_create;

    data_create.size = L2_SIZE;
    phys = (uint64_t)val_host_mem_alloc(L2_SIZE, (2 * data_create.size));
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

    ret = val_host_rmi_rtt_fold(realm[VALID_REALM].rd, data_create.ipa, MAP_LEVEL, &rtt);
    if (ret)
    {
        LOG(ERROR, "\tRTT_FOLD failed with ret value = %x\n", ret, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return data_create.ipa;
}

static uint64_t g_rec_ready_prep_sequence(void)
{
    val_host_rec_params_ts rec_params;

    realm[VALID_REALM].rec_count = 1;
    rec_params.pc = 0;
    rec_params.flags = RMI_RUNNABLE;

    if (val_host_rec_create_common(&realm[VALID_REALM], &rec_params))
    {
        LOG(ERROR, "\tCouldn't destroy the Realm\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm[VALID_REALM].rec[0];
}

static uint64_t ipa_valid_prep_sequence(void)
{
    uint64_t ret, phys;
    val_data_create_ts data_create;

    data_create.size = L2_SIZE;
    phys = (uint64_t)val_host_mem_alloc(L2_SIZE, (2 * data_create.size));
    if (!phys)
    {
        LOG(ERROR, "\tval_host_mem_alloc failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    data_create.src_pa = phys;
    data_create.target_pa = phys + data_create.size;
    data_create.ipa = IPA_HOMOGENEOUS_RTT;
    data_create.rtt_alignment = L2_SIZE;
    ret = val_host_map_protected_data_to_realm(&realm[VALID_REALM], &data_create);
    if (ret)
    {
        LOG(ERROR, "\tval_host_map_protected_data_to_realm failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return data_create.ipa;
}

static uint64_t g_rd_new_prep_sequence(uint16_t vmid)
{
    val_memset(&realm[VALID_REALM], 0, sizeof(realm[VALID_REALM]));

    realm[VALID_REALM].s2sz = 40;
    realm[VALID_REALM].hash_algo = RMI_HASH_SHA_256;
    realm[VALID_REALM].s2_starting_level = 0;
    realm[VALID_REALM].num_s2_sl_rtts = 1;
    realm[VALID_REALM].vmid = vmid;

    if (val_host_realm_create_common(&realm[VALID_REALM]))
    {
        LOG(ERROR, "\tRealm create failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm[VALID_REALM].rd;
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
            args->rd = g_rec_ready_prep_sequence();
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
            args->rd = g_data_prep_sequence(c_args.rd_valid, IPA_ADDR_DATA);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case LEVEL_STARTING_LEVLE:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = L_STARTING_LEVEL;
            break;

        case LEVEL_L1:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = L_1;
            break;

        case LEVEL_L2:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = L_2;
            break;

        case LEVEL_OUT_OF_BOUND:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = L_OUT_OF_BOUND;
            break;

        case IPA_L2_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->ipa = PAGE_SIZE;
            args->level = c_args.level_valid;
            break;

        case IPA_OUT_OF_BOUND:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_outside_of_permitted_ipa_prep_sequence();
            args->level = c_args.level_valid;
            break;

        case IPA_NOT_MAPPED:
            args->rd = c_args.rd_valid;
            args->ipa = IPA_UNMAPPED;
            args->level = c_args.level_valid;
            break;

        case RTTE_UNASSIGED:
            args->rd = c_args.rd_valid;
            args->ipa = 2 * L2_SIZE;
            args->level = c_args.level_valid;
            break;

        case RTTE_ASSIGNED:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_folded_rtt_prep_sequence();
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->level = c_args.level_valid;
            break;

        case RTT_NON_HOMOGENEOUS:
            args->rd = c_args.rd_valid;
            args->ipa = IPA_ADDR_DATA;
            args->level = c_args.level_valid;
            break;

        case LEVEL_BOUND_RTT_WALK:
            args->rd = c_args.rd_valid;
            args->ipa = IPA_UNMAPPED;
            args->level = L_STARTING_LEVEL;
            break;

        case LEVEL_BOUND_RTTE_STATE:
            args->rd = c_args.rd_valid;
            args->ipa = IPA_UNMAPPED;
            args->level = L_STARTING_LEVEL;
            break;

        default:
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_rtt_fold_host(void)
{
    uint64_t ret, rtt, i;
    struct arguments args;
    val_host_rtt_entry_ts rtte, fold;

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

        ret = val_host_rmi_rtt_fold(args.rd, args.ipa, args.level, &rtt);
        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    /* Save Parent rtte.addr for comparision */
    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                                         c_args.level_valid - 1, &rtte);
    if (ret) {
        LOG(ERROR, "\n\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    c_exp_output.rtt = OA(rtte.desc);

    /* Save fold.addr, fold.ripas for Positive Observability */
    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                                         c_args.level_valid, &fold);
    if (ret) {
        LOG(ERROR, "\n\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);
    ret = val_host_rmi_rtt_fold(c_args.rd_valid, c_args.ipa_valid, c_args.level_valid, &rtt);
    if (ret != 0)
    {
        LOG(ERROR, "\n\t RTT_FOLD failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    if (rtt != c_exp_output.rtt)
    {
        LOG(TEST, "\n\tUnexpected command output value. rtt : 0x%x\n", rtt, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    /* Compare rtte_addr, rtte_ripas in folded RTTE */
    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                                         c_args.level_valid - 1, &rtte);
    if (ret) {
        LOG(ERROR, "\n\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    /* Compare HIPAS, RIPAS and addr of parent RTTE to its child RTTE */
    if (rtte.state != fold.state || rtte.ripas != fold.ripas || rtte.desc != fold.desc)
    {
        LOG(TEST, "\n\t Parent RTTE does not match with child RTTE \n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

exit:

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                                         VAL_RTT_MAX_LEVEL, &rtte);

    if (ret) {
        LOG(ERROR, "\n\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        return;
    }

    if (rtte.walk_level != realm[VALID_REALM].s2_starting_level) {
        /* Unfold folded RTTs to satisfy postamble requirements */
        ret = val_host_create_rtt_levels(&realm[VALID_REALM], c_args.ipa_valid,
                                    (uint32_t)rtte.walk_level, VAL_RTT_MAX_LEVEL,
                                    L2_SIZE);
        if (ret)
        {
            LOG(ERROR, "\tval_host_create_rtt_level failed\n", 0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
            return;
        }
    }

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, IPA_FOLDED_RTT,
                                                         VAL_RTT_MAX_LEVEL, &rtte);

    if (ret) {
        LOG(ERROR, "\n\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
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
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
            return;
        }
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    return;
}
