/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_rec_aux_count_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40

#define NUM_REALMS 1
#define REALM_VALID 0

#define IPA_ADDR_DATA 0

static val_host_realm_ts realm_test[NUM_REALMS];

static struct argument_store {
    uint64_t rd_valid;
} c_args;

struct arguments {
    uint64_t rd;
};

static uint64_t g_rd_new_prep_sequence(uint16_t vmid)
{
    val_host_realm_ts realm_init;

    val_memset(&realm_init, 0, sizeof(realm_init));

    realm_init.s2sz = 40;
    realm_init.hash_algo = RMI_HASH_SHA_256;
    realm_init.s2_starting_level = 0;
    realm_init.num_s2_sl_rtts = 1;
    realm_init.vmid = vmid;
    realm_init.rec_count = 1;

    if (val_host_realm_create_common(&realm_init))
    {
        LOG(ERROR, "\tRealm create failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    realm_test[vmid].rd = realm_init.rd;
    realm_test[vmid].rtt_l0_addr = realm_init.rtt_l0_addr;
    return realm_init.rd;
}

static uint64_t rd_valid_prep_sequence(void)
{
    return g_rd_new_prep_sequence(REALM_VALID);
}

static uint64_t g_rec_ready_prep_sequence(void)
{
    val_host_rec_params_ts rec_params;

    rec_params.pc = 0;
    rec_params.flags = RMI_RUNNABLE;
    rec_params.mpidr = 0;

    if (val_host_rec_create_common(&realm_test[REALM_VALID], &rec_params)) {
        LOG(ERROR, "\n\t REC Create Failed \n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm_test[REALM_VALID].rec[0];
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

        case RD_DEV_MEM:
            args->rd = g_dev_mem_prep_sequence();
            break;

        case RD_OUTSIDE_OF_PERMITTED_PA:
            args->rd = g_outside_of_permitted_pa_prep_sequence();
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
            args->rd = g_rec_ready_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case RD_STATE_RTT:
            args->rd = realm_test[REALM_VALID].rtt_l0_addr;
            break;

        case RD_STATE_DATA:
            args->rd = g_data_prep_sequence(c_args.rd_valid, IPA_ADDR_DATA);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        default:
            /* set status to failure */
            LOG(ERROR, "\t\nUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}


void cmd_rec_aux_count_host(void)
{
    uint64_t ret;
    struct arguments args;
    uint64_t i, value = 0;

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

        ret = val_host_rmi_rec_aux_count(args.rd, &value);

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index))
        {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);

    ret = val_host_rmi_rec_aux_count(c_args.rd_valid, &value);
    /* Valid call should give success if footprints have not changed */
    if (ret != 0)
    {
        LOG(TEST, "\t\nERROR\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
