/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_realm_activate_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define MAX_GRANULES 256

#define NUM_REALMS 3
#define VALID_REALM 0
#define NULL_REALM 1
#define SYSTEM_OFF_REALM 2

#define IPA_ADDR_DATA  (4 * PAGE_SIZE)

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

    if (val_host_realm_create_common(&realm_init))
    {
        LOG(ERROR, "\tRealm create failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    realm_test[vmid].rd = realm_init.rd;
    realm_test[vmid].rtt_l0_addr = realm_init.rtt_l0_addr;
    return realm_init.rd;
}

static uint64_t rd_valid_prep_sequence(void)
{
    return g_rd_new_prep_sequence(VALID_REALM);
}

static uint64_t g_rd_null_prep_sequence(void)
{
    uint64_t rd;
    rd = g_rd_new_prep_sequence(NULL_REALM);
    if (rd == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    if (val_host_rmi_realm_destroy(rd))
    {
        LOG(ERROR, "\tCouldn't destroy the Realm\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    return rd;
}

static uint64_t g_rec_ready_prep_sequence(uint64_t rd)
{
    val_host_realm_ts realm;
    val_host_rec_params_ts rec_params;

    realm.rec_count = 1;
    realm.rd = rd;
    rec_params.pc = 0;
    rec_params.flags = RMI_RUNNABLE;
    rec_params.mpidr = 0;

    if (val_host_rec_create_common(&realm, &rec_params))
    {
        LOG(ERROR, "\tCouldn't destroy the Realm\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm.rec[0];
}

static uint64_t g_rd_system_off_prep_sequence(void)
{
    uint64_t ret;
    val_memset(&realm_test[SYSTEM_OFF_REALM], 0, sizeof(realm_test[SYSTEM_OFF_REALM]));

    val_host_realm_params(&realm_test[SYSTEM_OFF_REALM]);

    realm_test[SYSTEM_OFF_REALM].vmid = SYSTEM_OFF_REALM;

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm_test[SYSTEM_OFF_REALM], 1))
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm_test[SYSTEM_OFF_REALM].rec[0],
                                realm_test[SYSTEM_OFF_REALM].run[0]);
    if (ret)
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
    return realm_test[SYSTEM_OFF_REALM].rd;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.rd_valid = rd_valid_prep_sequence();
    if (c_args.rd_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_ERROR;

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

        case RD_DEV_MEM_MMIO:
            args->rd = g_dev_mem_prep_sequence();;
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

        case RD_REC:
            args->rd = g_rec_ready_prep_sequence(c_args.rd_valid);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case RD_DATA:
            args->rd = g_data_prep_sequence(c_args.rd_valid, IPA_ADDR_DATA);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case RD_RTT:
            args->rd = realm_test[VALID_REALM].rtt_l0_addr;
            break;

        case REALM_SYSTEM_OFF:
            args->rd = g_rd_system_off_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case REALM_NULL:
            args->rd = g_rd_null_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        default:
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_realm_activate_host(void)
{
    uint64_t ret, i;
    struct arguments args;

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto fail;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "\n\tCheck %d : ", i + 1, 0);
        LOG(TEST, test_data[i].msg, 0, 0);
        LOG(TEST, "; intent id : 0x%x \n", test_data[i].label, 0);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
            goto fail;
        }

        ret = val_host_rmi_realm_activate(args.rd);

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index))
        {
            LOG(ERROR, "\tret %x\n", ret, 0);
            LOG(ERROR, "\tERROR status code : %d index %d\n", test_data[i].status,
                                                              test_data[i].index);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
            goto fail;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);
    ret = val_host_rmi_realm_activate(c_args.rd_valid);
    if (ret != 0)
    {
        LOG(ERROR, "\n\tRealm activate command failed. %x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto fail;
    }

    LOG(TEST, "\n\tNegative Observability Check\n", 0, 0);
    ret = val_host_rmi_realm_activate(c_args.rd_valid);
    if (ret != PACK_CODE(RMI_ERROR_REALM, 0))
    {
        LOG(ERROR, "\n\tRealm activate command should fail. %x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto fail;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));
    return;

fail:
    val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
    return;
}
