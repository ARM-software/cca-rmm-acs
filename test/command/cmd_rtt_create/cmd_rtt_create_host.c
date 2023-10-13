/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_rtt_create_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define START_RTT_LEVEL 0
#define MAX_RTT_LEVEL 3
#define MAX_GRANULES 256

#define MAP_LEVEL 2
#define WALK_ERR_LEVEL 3
#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512L * L3_SIZE)
#define L1_SIZE (512L * L2_SIZE)
#define L0_SIZE (512L * L1_SIZE)

/*      Valid Realm memory layout
 *
 * ipa: 0x0 (1st <START_LEVEL> Table Entry)
 *  ------------------------------------
 * |  HIPAS = UNASSIGNED, RIPAS = EMPTY |
 *  ------------------------------------
 * |        RTTE.state = TABLE          | -> mappings created up to leaf level
 *  ------------------------------------
 * ipa: 2 * SL_SIZE
 *
 */

#define IPA_ADDR_PROTECTED_UNMAPPED L1_SIZE
#define IPA_ADDR_DATA  4 * PAGE_SIZE
#define IPA_ADDR_DATA_1  (IPA_ADDR_DATA + PAGE_SIZE)

#define NUM_REALMS 1
#define VALID_REALM 0

static val_host_realm_ts realm_test[NUM_REALMS];

static struct argument_store {
    uint64_t rd_valid;
    uint64_t rtt_valid;
    uint64_t ipa_valid;
    uint64_t level_valid;
} c_args;

static struct invalid_arguments {
    uint64_t rec_gran;
} c_args_invalid;

struct arguments {
    uint64_t rd;
    uint64_t rtt;
    uint64_t ipa;
    uint64_t level;
};

static uint64_t rtt_valid_prep_sequence(void)
{
    return g_delegated_prep_sequence();
}

static uint64_t g_rd_new_prep_sequence(uint16_t vmid)
{
    val_host_realm_ts realm_init;

    val_memset(&realm_init, 0, sizeof(realm_init));

    realm_init.s2sz = 40;
    realm_init.hash_algo = RMI_HASH_SHA_256;
    realm_init.s2_starting_level = 0;
    realm_init.num_s2_sl_rtts = 1;
    realm_init.vmid = vmid;

    if (val_host_realm_create_common(&realm_init))
    {
        LOG(ERROR, "\tRealm create failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    realm_test[vmid].rd = realm_init.rd;
    realm_test[vmid].rtt_l0_addr = realm_init.rtt_l0_addr;
    return realm_init.rd;
}

static uint64_t ipa_level_1_unaligned_prep_sequence(void)
{
    return 2 * L2_SIZE;
}
static uint64_t rd_valid_prep_sequence(void)
{
    return g_rd_new_prep_sequence(VALID_REALM);
}

static uint64_t ipa_valid_prep_sequence(void)
{
    /* RTTE[ipa].state = UNASSIGNED
       RTTE[ipa].ripas = EMPTY */
    return IPA_ADDR_PROTECTED_UNMAPPED;
}


static uint64_t level_valid_prep_sequence(void)
{
    return MAP_LEVEL;
}

static uint64_t level_invalid_start_prep_sequence(void)
{
    return START_RTT_LEVEL;
}


static uint64_t level_invalid_oob_prep_sequence(void)
{
    return MAX_RTT_LEVEL + 1;
}

static uint64_t level_no_parent_rtt_prep_sequence(void)
{
    /* Attempt to create an RTT at a level & ipa for which no parent RTT Entry exists */
    return WALK_ERR_LEVEL;
}

static uint64_t g_rec_ready_prep_sequence(uint64_t rd)
{
    val_host_realm_ts realm;
    val_host_rec_params_ts rec_params;

    realm.rec_count = 1;
    realm.rd = rd;
    rec_params.pc = 0;
    rec_params.flags = RMI_RUNNABLE;

    if (val_host_rec_create_common(&realm, &rec_params))
    {
        LOG(ERROR, "\tREC Create Failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm.rec[0];
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.rtt_valid = rtt_valid_prep_sequence();
    if (c_args.rtt_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.rd_valid = rd_valid_prep_sequence();
    if (c_args.rd_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.ipa_valid = ipa_valid_prep_sequence();

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
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_DEV_MEM:
            args->rd = g_dev_mem_prep_sequence();
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

       case RD_OUTSIDE_OF_PERMITTED_PA:
            args->rd = g_outside_of_permitted_pa_prep_sequence();
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_STATE_DELEGATED:
            args->rd = g_delegated_prep_sequence();
            if  (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_STATE_REC:
            args->rd = g_rec_ready_prep_sequence(c_args.rd_valid);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->rtt = c_args.rtt_valid;
            c_args_invalid.rec_gran = args->rd; /* Storing this for future reference */
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_STATE_RTT:
            args->rd = realm_test[VALID_REALM].rtt_l0_addr;
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_STATE_DATA:
            args->rd = g_data_prep_sequence(c_args.rd_valid, IPA_ADDR_DATA);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case LEVEL_STARTING:
            args->rd = c_args.rd_valid;
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = level_invalid_start_prep_sequence();
            break;

        case LEVEL_OOB:
            args->rd = c_args.rd_valid;
            args->rtt = c_args.rtt_valid;
            /* Note: we require the RTT walk to succeed here */
            args->ipa = ipa_unprotected_unassigned_prep_sequence(c_args.rd_valid);
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->level = level_invalid_oob_prep_sequence();
            break;

        case IPA_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rtt = c_args.rtt_valid;
            args->ipa = ipa_level_1_unaligned_prep_sequence();
            args->level = c_args.level_valid;
            break;

        case IPA_OOB:
            args->rd = c_args.rd_valid;
            args->rtt = c_args.rtt_valid;
            args->ipa = ipa_outside_of_permitted_ipa_prep_sequence();
            args->level = c_args.level_valid;
            break;

        case RTT_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rtt = g_unaligned_prep_sequence(c_args.rtt_valid);
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RTT_DEV_MEM:
            args->rd = c_args.rd_valid;
            args->rtt = g_dev_mem_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RTT_OUTSIDE_PERMITTED_PA:
            args->rd = c_args.rd_valid;
            args->rtt = g_outside_of_permitted_pa_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RTT_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->rtt = g_undelegated_prep_sequence();
            if (args->rtt == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RTT_STATE_RD:
            args->rd = c_args.rd_valid;
            args->rtt = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RTT_STATE_REC:
            args->rd = c_args.rd_valid;
            args->rtt = c_args_invalid.rec_gran;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RTT_STATE_RTT:
            args->rd = c_args.rd_valid;
            args->rtt = realm_test[VALID_REALM].rtt_l0_addr;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RTT_STATE_DATA:
            args->rd = c_args.rd_valid;
            args->rtt = g_data_prep_sequence(c_args.rd_valid, IPA_ADDR_DATA_1);
            if (args->rtt == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case LEVEL_NO_PARENT_RTTE:
            args->rd = c_args.rd_valid;
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = level_no_parent_rtt_prep_sequence();
            break;

        case RTTE_STATE_TABLE:
            args->rd = c_args.rd_valid;
            args->rtt = c_args.rtt_valid;
            args->ipa = ipa_unprotected_assinged_prep_sequence(c_args.rd_valid);
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->level = c_args.level_valid;
            break;

        default:
            /* set status to failure */
            LOG(ERROR, "\nUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }
    return VAL_SUCCESS;
}


void cmd_rtt_create_host(void)
{
    uint64_t ret = 0;
    struct arguments args;
    val_host_rtt_entry_ts rtte;
    uint64_t i;

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    /* Iterate over the input */
    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "\n\tCheck %d : ", i + 1, 0);
        LOG(TEST, test_data[i].msg, 0, 0);
        LOG(TEST, "; intent id : 0x%x \n", test_data[i].label, 0);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        ret = val_host_rmi_rtt_create(args.rd, args.rtt, args.ipa, args.level);

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    /* Check that rtte.addr and rtte.state have not changed */
    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                      (c_args.level_valid - 1), &rtte);
    if (ret) {
        LOG(ERROR, "\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    } else {
        if (rtte.state != RMI_UNASSIGNED || OA(rtte.desc) == c_args.rtt_valid) {
            LOG(ERROR, "\tUnexptected RTT entry received\n", 0, 0);
            LOG(ERROR, "\tState is: %d & OA is: %x\n", rtte.state, OA(rtte.desc));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);

    ret = val_host_rmi_rtt_create(c_args.rd_valid, c_args.rtt_valid,
                              c_args.ipa_valid, c_args.level_valid);

    /* Valid call should give success if footprints have not changed */
    if (ret != 0) {
        LOG(ERROR, "\n\tPositive Observability failed ", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    /* Check that rtte.addr and rtte.state have changed */
    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                      (c_args.level_valid - 1), &rtte);
    if (ret) {
        LOG(ERROR, "\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    } else {
        if (rtte.state != RMI_TABLE || OA(rtte.desc) != c_args.rtt_valid) {
            LOG(ERROR, "\tUnexptected RTT entry received\n", 0, 0);
            LOG(ERROR, "\tState is: %d & OA is: %x\n", rtte.state, OA(rtte.desc));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
            goto exit;
        }
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
