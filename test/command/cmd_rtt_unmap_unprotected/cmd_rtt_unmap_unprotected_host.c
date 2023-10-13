/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_rtt_unmap_unprotected_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512 * L3_SIZE)
#define L1_SIZE (512 * L2_SIZE)
#define L0_SIZE (512UL * L1_SIZE)

#define NUM_REALMS 1
#define VALID_REALM 0
#define IPA_ADDR_DATA  5 * PAGE_SIZE
#define IPA_ADDR_UNPROTECTED (1UL << (IPA_WIDTH - 1))
#define IPA_L1_NOT_MAPPED IPA_ADDR_UNPROTECTED + L0_SIZE

#define START_RTT_LEVEL 0
#define MAX_RTT_LEVEL 3
#define MAP_LEVEL 3

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
    uint64_t top;
} c_exp_output;

static uint64_t ipa_l1_rtt_not_mapped_prep_sequence(void)
{
    return IPA_L1_NOT_MAPPED;
}

static uint64_t level_invalid_start_prep_sequence(void)
{
    return START_RTT_LEVEL;
}

static uint64_t level_invalid_oob_prep_sequence(void)
{
    return MAX_RTT_LEVEL + 1;
}

static uint64_t level_invalid_l1_prep_sequence(void)
{
    return 1;
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

static uint64_t level_valid_prep_sequence(void)
{
    return MAP_LEVEL;
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

    c_args.level_valid = level_valid_prep_sequence();

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

        case LEVEL_STARTING:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = level_invalid_start_prep_sequence();
            break;

        case LEVEL_L1:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = level_invalid_l1_prep_sequence();
            break;

        case LEVEL_OOB:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = level_invalid_oob_prep_sequence();
            break;

        case IPA_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->ipa = g_unaligned_prep_sequence(c_args.ipa_valid);
            args->level = c_args.level_valid;
            break;

        case IPA_PROTECTED:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_protected_unassigned_empty_prep_sequence(realm[VALID_REALM].rd);
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->level = c_args.level_valid;
            break;

        case IPA_OOB:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_outside_of_permitted_ipa_prep_sequence();
            args->level = c_args.level_valid;
            break;

        case IPA_NOT_MAPPED:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_unprotected_unmapped_prep_sequence();
            args->level = c_args.level_valid;
            c_exp_output.top = args->ipa + L2_SIZE;
            break;

        case RTTE_STATE_UNASSIGNED_NS:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_unprotected_unassigned_prep_sequence(realm[VALID_REALM].rd);
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->level = c_args.level_valid;
            c_exp_output.top = IPA_ADDR_UNPROTECTED + L3_SIZE;
            break;

        case LEVEL_L1_IPA_NOT_MAPPED:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_l1_rtt_not_mapped_prep_sequence();
            args->level = level_invalid_l1_prep_sequence();
            break;

        case LEVEL_L1_RTTE_STATE_TABLE:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_unprotected_unassigned_prep_sequence(realm[VALID_REALM].rd);
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->level = level_invalid_l1_prep_sequence();
            break;

        case IPA_PROTECTED_IPA_NOT_MAPPED:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_protected_unmapped_prep_sequence();
            args->level = c_args.level_valid;
            break;

        default:
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_rtt_unmap_unprotected_host(void)
{
    uint64_t ret = 0, i, top = 0;
    struct arguments args;
    val_host_rtt_entry_ts rtte;

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

        ret = val_host_rmi_rtt_unmap_unprotected(args.rd, args.ipa, args.level, &top);
        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }

        /* Upon RMI_ERROR_RTT check for top == walk_top */
        else if (ret == RMI_ERROR_RTT && top != c_exp_output.top) {
            LOG(ERROR, " \tUnexpected command output received, top: 0x%x\n", top, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);

    ret = val_host_rmi_rtt_unmap_unprotected(c_args.rd_valid, c_args.ipa_valid,
                                                                     c_args.level_valid, &top);
    if (ret != 0)
    {
        LOG(ERROR, "\n\t RTT_UNMAP_UNPROTECTED failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    if (top != (IPA_ADDR_UNPROTECTED + L2_SIZE)) {
        LOG(ERROR, "\n\tUnexpected Command Output : top  %x\n", top, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    /* Upon successful execution of the command, rtte.state should read UNASSIGNED */
    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                                                 c_args.level_valid, &rtte);
    if (ret) {
        LOG(ERROR, "\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    if (rtte.state != RMI_UNASSIGNED)
    {
        LOG(ERROR, "\n\t Unexpected RTTE state, State is: %d\n", rtte.state, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
