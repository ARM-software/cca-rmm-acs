/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_realm_destroy_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512 * L3_SIZE)
#define L1_SIZE (512 * L2_SIZE)
#define L0_SIZE (512UL * L1_SIZE)

#define NUM_REALMS 3
#define VALID_REALM 0
#define LIVE_REALM 1
#define NULL_REALM 2

#define MAP_LEVEL 3
#define RTT_STARTING_LEVEL 0
#define IPA_ADDR_DATA 4 * PAGE_SIZE

static val_host_realm_ts realm[NUM_REALMS];
static uint64_t level_size[4] = {L0_SIZE, L1_SIZE,
                                L2_SIZE, L3_SIZE};

static struct argument_store {
    uint64_t rd_valid;
} c_args;

struct arguments {
    uint64_t rd;
};


static uint64_t g_rec_ready_prep_sequence(void)
{
    val_host_rec_params_ts rec_params;

    val_memset(&realm[LIVE_REALM], 0, sizeof(realm[LIVE_REALM]));
    val_memset(&rec_params, 0, sizeof(rec_params));

    realm[LIVE_REALM].s2sz = IPA_WIDTH;
    realm[LIVE_REALM].hash_algo = RMI_HASH_SHA_256;
    realm[LIVE_REALM].s2_starting_level = 0;
    realm[LIVE_REALM].num_s2_sl_rtts = 1;
    realm[LIVE_REALM].vmid = LIVE_REALM;
    realm[LIVE_REALM].rec_count = 1;

    if (val_host_realm_create_common(&realm[LIVE_REALM]))
    {
        LOG(ERROR, "\tRealm create failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (val_host_rec_create_common(&realm[LIVE_REALM], &rec_params))
    {
        LOG(ERROR, "\tRec create failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm[LIVE_REALM].rec[0];
}

static uint64_t g_rd_null_prep_sequence(void)
{
    val_memset(&realm[NULL_REALM], 0, sizeof(realm[NULL_REALM]));

    realm[NULL_REALM].s2sz = 40;
    realm[NULL_REALM].hash_algo = RMI_HASH_SHA_256;
    realm[NULL_REALM].s2_starting_level = 0;
    realm[NULL_REALM].num_s2_sl_rtts = 1;
    realm[NULL_REALM].vmid = NULL_REALM;
    realm[NULL_REALM].rec_count = 1;

    if (val_host_realm_create_common(&realm[NULL_REALM]))
    {
        LOG(ERROR, "\tRealm create failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (val_host_rmi_realm_destroy(realm[NULL_REALM].rd))
    {
        LOG(ERROR, "\tCouldn't destroy the Realm\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm[NULL_REALM].rd;
}

/* Valid RD for REALM_DESTROY is a non-live rd whose RTTs (except starting level), REC's
 * and DATA granules are destroyed */
static uint64_t rd_valid_prep_sequence(void)
{
    val_host_rec_params_ts rec_params;
    uint64_t ipa, level = MAP_LEVEL, ipa_aligned;
    val_host_rtt_destroy_ts out_val;

    val_memset(&realm[VALID_REALM], 0, sizeof(realm[VALID_REALM]));
    val_memset(&rec_params, 0, sizeof(rec_params));

    realm[VALID_REALM].s2sz = 40;
    realm[VALID_REALM].hash_algo = RMI_HASH_SHA_256;
    realm[VALID_REALM].s2_starting_level = 0;
    realm[VALID_REALM].num_s2_sl_rtts = 1;
    realm[VALID_REALM].vmid = VALID_REALM;
    realm[VALID_REALM].rec_count = 1;

    if (val_host_realm_create_common(&realm[VALID_REALM]))
    {
        LOG(ERROR, "\tRealm create failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (val_host_rec_create_common(&realm[VALID_REALM], &rec_params))
    {
        LOG(ERROR, "\tRec create failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    ipa = ipa_protected_destroyed_prep_sequence(realm[VALID_REALM].rd);
    if (ipa == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    while (level > RTT_STARTING_LEVEL)
    {
        ipa_aligned = ipa / level_size[level - 1] * level_size[level - 1];

        if (val_host_rmi_rtt_destroy(realm[VALID_REALM].rd, ipa_aligned, level, &out_val))

        {
            LOG(ERROR, "\tCouldn't destroy RTTs\n", 0, 0);
            return VAL_TEST_PREP_SEQ_FAILED;
        }
        level--;
    }

    if (val_host_rmi_rec_destroy(realm[VALID_REALM].rec[0]))
    {
        LOG(ERROR, "\tCouldn't destroy REC\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm[VALID_REALM].rd;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.rd_valid = rd_valid_prep_sequence();
    if (c_args.rd_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            break;

        case RD_OUTSIDE_OF_PERMITTED_PA:
            args->rd = g_outside_of_permitted_pa_prep_sequence();
            break;

        case RD_DEV_MEM:
            args->rd = g_dev_mem_prep_sequence();
            break;

        case RD_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case RD_STATE_DELEGATED:
            args->rd = g_rd_null_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case RD_STATE_REC:
            args->rd = g_rec_ready_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case RD_STATE_RTT:
            args->rd = realm[LIVE_REALM].rtt_l0_addr;
            break;

        case RD_STATE_DATA:
            args->rd = g_data_prep_sequence(realm[LIVE_REALM].rd, IPA_ADDR_DATA);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case REALM_LIVE:
            args->rd = realm[LIVE_REALM].rd;
            break;

        default:
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_realm_destroy_host(void)
{
    uint64_t ret, i;
    struct arguments args;
    val_host_realm_params_ts *params;

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

        ret = val_host_rmi_realm_destroy(args.rd);
        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);
    ret = val_host_rmi_realm_destroy(c_args.rd_valid);
    if (ret != 0)
    {
        LOG(ERROR, "\n\tREALM_DESTROY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    /* When command suceeds, rtt_state, rd_state changes to DELEGATED. Upon attempting to
     * create a realm with same rd and rtt l0 address, REALM_CREATE should suceed */

    /* Allocate memory for params */
    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (params == NULL)
    {
        LOG(ERROR, "\tFailed to allocate memory for params\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    val_memset(params, 0, PAGE_SIZE);
    /* Populate params */
    params->rtt_base = realm[VALID_REALM].rtt_l0_addr;
    params->hash_algo = realm[VALID_REALM].hash_algo;
    params->s2sz = realm[VALID_REALM].s2sz;
    params->rtt_level_start = realm[VALID_REALM].s2_starting_level;
    params->rtt_num_start = realm[VALID_REALM].num_s2_sl_rtts;
    params->vmid = realm[VALID_REALM].vmid;

    if (val_host_rmi_realm_create(c_args.rd_valid, (uint64_t)params))
    {
        LOG(ERROR, "\tPositive observability failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
