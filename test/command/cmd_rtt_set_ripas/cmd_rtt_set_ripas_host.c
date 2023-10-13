/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_rtt_set_ripas_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512 * L3_SIZE)
#define L1_SIZE (512 * L2_SIZE)
#define MAP_LEVEL 3

#define NUM_REALMS 2
#define VALID_REALM 0
#define INVALID_REALM 1

static val_host_realm_ts realm[NUM_REALMS];
static val_host_rec_exit_ts *rec_exit;

static struct argument_store {
    uint64_t rd_valid;
    uint64_t rec_valid;
    uint64_t base_valid;
    uint64_t top_valid;
} c_args;

struct arguments {
    uint64_t rd;
    uint64_t rec;
    uint64_t base;
    uint64_t top;
};

static uint64_t g_rec_other_owner_prep_sequence(void)
{
    val_host_rec_params_ts rec_params;

    val_memset(&realm[INVALID_REALM], 0, sizeof(realm[INVALID_REALM]));

    realm[INVALID_REALM].s2sz = 40;
    realm[INVALID_REALM].hash_algo = RMI_HASH_SHA_256;
    realm[INVALID_REALM].s2_starting_level = 0;
    realm[INVALID_REALM].num_s2_sl_rtts = 1;
    realm[INVALID_REALM].vmid = 1;
    realm[INVALID_REALM].rec_count = 1;

    rec_params.pc = 0;
    rec_params.flags = RMI_RUNNABLE;


    if (val_host_realm_create_common(&realm[INVALID_REALM]))
    {
        LOG(ERROR, "\tRealm create failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Populate realm with two RECs*/
    if (val_host_rec_create_common(&realm[INVALID_REALM], &rec_params))
    {
        LOG(ERROR, "\tREC Create Failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm[INVALID_REALM].rec[0];
}

static uint64_t rd_valid_prep_sequence(void)
{
    uint64_t ret, ipa_base;

    val_memset(&realm[VALID_REALM], 0, sizeof(realm[VALID_REALM]));

    val_host_realm_params(&realm[VALID_REALM]);

    realm[VALID_REALM].vmid = VALID_REALM;
    realm[VALID_REALM].rec_count = 2;

    /* Populate realm with two RECs*/
    if (val_host_realm_setup(&realm[VALID_REALM], 1))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm[VALID_REALM].rec[0], realm[VALID_REALM].run[0]);
    if (ret)
    {
        LOG(ERROR, "\tREC_ENTER failed with ret value: %d\n", ret, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    } else if (val_host_check_realm_exit_ripas_change(
                                    (val_host_rec_run_ts *)realm[VALID_REALM].run[0]))
    {
        LOG(ERROR, "\tRipas change req failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    rec_exit =  &(((val_host_rec_run_ts *)realm[VALID_REALM].run[0])->exit);
    ipa_base = rec_exit->ripas_base;

    /* Create RTT mappings for requested IPA range in case it doesn't exist */
    for (uint8_t i = 0; i < rec_exit->ripas_top / PAGE_SIZE ; i++) {
        if (create_mapping(ipa_base, false, realm[VALID_REALM].rd))
            return VAL_TEST_PREP_SEQ_FAILED;
        ipa_base += PAGE_SIZE;
    }

    return realm[VALID_REALM].rd;
}

static uint64_t base_unaligned_prep_sequence(void)
{
    uint64_t ret;

    ret = val_host_rmi_rec_enter(realm[VALID_REALM].rec[1], realm[VALID_REALM].run[1]);
    if (ret)
    {
        LOG(ERROR, "\tREC_ENTER failed with ret value: %d\n", ret, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    } else if (val_host_check_realm_exit_ripas_change(
                                    (val_host_rec_run_ts *)realm[VALID_REALM].run[1]))
    {
        LOG(ERROR, "\tRipas change req failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    rec_exit =  &(((val_host_rec_run_ts *)realm[VALID_REALM].run[1])->exit);

    return rec_exit->ripas_base;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.rd_valid = rd_valid_prep_sequence();
    if (c_args.rd_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.rec_valid = realm[VALID_REALM].rec[0];

    c_args.base_valid = rec_exit->ripas_base;

    c_args.top_valid = rec_exit->ripas_top;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            args->rec = c_args.rec_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_OUTSIDE_OF_PERMITTED_PA:
            args->rd = g_outside_of_permitted_pa_prep_sequence();
            args->rec = c_args.rec_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_DEV_MEM:
            args->rd = g_dev_mem_prep_sequence();
            args->rec = c_args.rec_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->rec = c_args.rec_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_STATE_DELEGATED:
            args->rd = g_delegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->rec = c_args.rec_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_STATE_REC:
            args->rd = realm[VALID_REALM].rec[0];
            args->rec = c_args.rec_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_STATE_RTT:
            args->rd = realm[VALID_REALM].rtt_l0_addr;
            args->rec = c_args.rec_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_STATE_DATA:
            args->rd = realm[VALID_REALM].image_pa_base;
            args->rec = c_args.rec_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rec = g_unaligned_prep_sequence(c_args.rec_valid);
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_OUTSIDE_OF_PERMITTED_PA:
            args->rd = c_args.rd_valid;
            args->rec = g_outside_of_permitted_pa_prep_sequence();
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_DEV_MEM:
            args->rd = c_args.rd_valid;
            args->rec = g_dev_mem_prep_sequence();
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_GRAN_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->rec = g_undelegated_prep_sequence();
            if (args->rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_GRAN_STATE_DELEGATED:
            args->rd = c_args.rd_valid;
            args->rec = g_delegated_prep_sequence();
            if (args->rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_GRAN_STATE_RD:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rd_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_GRAN_STATE_RTT:
            args->rd = c_args.rd_valid;
            args->rec = realm[VALID_REALM].rtt_l0_addr;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REc_GRAN_STATE_DATA:
            args->rd = c_args.rd_valid;
            args->rec = realm[VALID_REALM].image_pa_base;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_OTHER_OWNER:
            args->rd = c_args.rd_valid;
            args->rec = g_rec_other_owner_prep_sequence();
            if (args->rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case INVALID_SIZE:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rec_valid;
            args->base = c_args.base_valid;
            args->top = c_args.base_valid;
            break;

        case BASE_MISMATCH:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rec_valid;
            args->base = c_args.base_valid + 1;
            args->top = c_args.top_valid;
            break;

        case TOP_OUT_OF_BOUND:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rec_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid + PAGE_SIZE;
            break;

        case BASE_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rec = realm[VALID_REALM].rec[1];
            args->base = base_unaligned_prep_sequence();
            if (args->base == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->top = c_args.top_valid + L2_SIZE;
            break;

        case TOP_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rec_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid - PAGE_SIZE / 2;
            break;

       case BASE_MISMATCH_BASE_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rec_valid;
            args->base = c_args.base_valid + PAGE_SIZE / 2;
            args->top = c_args.top_valid;
            break;

        default:
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_rtt_set_ripas_host(void)
{
    uint64_t ret, i, ipa_base, out_top;
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

        ret = val_host_rmi_rtt_set_ripas(args.rd, args.rec, args.base, args.top, &out_top);
        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tNegative Observability Check\n", 0, 0);

    ipa_base = c_args.base_valid;
    for (uint8_t i = 0; i < c_args.top_valid / PAGE_SIZE ; i++) {
        ret = val_host_rmi_rtt_read_entry(c_args.rd_valid,
                                      ipa_base, MAP_LEVEL, &rtte);
        if (ret) {
            LOG(ERROR, "\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }

        if (rtte.walk_level == MAP_LEVEL && rtte.ripas != RMI_EMPTY) {
            LOG(ERROR, "\n\t Unexpected RTT entry. walk level: %d, RIPAS : %d",
                                                             rtte.walk_level, rtte.ripas);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto exit;
        }
        ipa_base += PAGE_SIZE;
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);
    ret = val_host_rmi_rtt_set_ripas(c_args.rd_valid, c_args.rec_valid, c_args.base_valid,
                                                               c_args.top_valid, &out_top);
    if (ret != 0)
    {
        LOG(ERROR, "\n\t RTT_SET_RIPAS failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    ipa_base = c_args.base_valid;
    for (uint8_t i = 0; i < c_args.top_valid / PAGE_SIZE ; i++) {
        ret = val_host_rmi_rtt_read_entry(c_args.rd_valid,
                                      ipa_base, MAP_LEVEL, &rtte);
        if (ret) {
            LOG(ERROR, "\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
            goto exit;
        }

        if (rtte.walk_level == MAP_LEVEL && rtte.ripas != RMI_RAM) {
            LOG(ERROR, "\n\t Unexpected RTT entry. walk level: %d, RIPAS : %d",
                                                             rtte.walk_level, rtte.ripas);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
            goto exit;
        }
        ipa_base += PAGE_SIZE;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
