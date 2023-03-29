/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_rtt_init_ripas_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define MAX_GRANULES 256

#define MAP_LEVEL 3
#define WALK_LEVEL_TABLE 2
#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512L * L3_SIZE)
#define L1_SIZE (512L * L2_SIZE)
#define L0_SIZE (512L * L1_SIZE)

/*      Valid Realm memory layout
 *
 * ipa: 0x0 (1st L2 Table Entry)
 *  ------------------------------------
 * |        RTTE.state = TABLE          | -> L3: HIPAS = ASSIGNED, RIPAS = RAM
 *  ------------------------------------
 * | HIPAS = UNASSIGNED, RIPAS = EMPTY  |
 *  ------------------------------------
 * ipa: 0x400000
 *
 */

#define IPA_ADDR_VALID 0
#define IPA_ADDR_DATA  (4 * PAGE_SIZE)

#define NUM_REALMS 4
#define MAX_REALMS 5

#define VALID_REALM 0
#define ACTIVE_REALM 1
#define NULL_REALM 2
#define SYSTEM_OFF_REALM 3

static val_host_realm_ts realm_test[NUM_REALMS];

static struct granule_store {
    uint64_t rd_valid;
    uint64_t ipa_valid;
    uint64_t level_valid;
} c_args;

struct arguments {
    uint64_t rd;
    uint64_t ipa;
    uint64_t level;
};

static uint64_t g_rd_new_prep_sequence(uint16_t vmid)
{
    val_host_realm_ts realm_init;
    val_host_rmifeatureregister0_ts features_0;

    val_memset(&realm_init, 0, sizeof(realm_init));
    val_memset(&features_0, 0, sizeof(features_0));
    features_0.s2sz = 40;
    val_memcpy(&realm_init.realm_feat_0, &features_0, sizeof(features_0));

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

static uint64_t rd_valid_prep_sequence(void)
{
    return g_rd_new_prep_sequence(VALID_REALM);
}

static uint64_t ipa_valid_prep_sequence(void)
{
    /* RTTE[ipa].state = UNASSIGNED
       RTTE[ipa].ripas = EMPTY */
    if (create_mapping(IPA_ADDR_VALID, false, c_args.rd_valid))
    {
        LOG(ERROR, "\tCouldn't create the protected mapping\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return IPA_ADDR_VALID;
}


static uint64_t level_valid_prep_sequence(void)
{
    // TODO: get start_level from VAL and derive MAP_LEVEL from that
    return MAP_LEVEL;
}

static uint64_t level_invalid_oob_prep_sequence(void)
{
    return  VAL_RTT_MAX_LEVEL + 1;
}

static uint64_t g_rd_active_prep_sequence(void)
{
    uint64_t rd;
    rd = g_rd_new_prep_sequence(ACTIVE_REALM);
    if (rd == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    if (val_host_rmi_realm_activate(rd))
    {
        LOG(ERROR, "\tCouldn't activate the Realm\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    return rd;
}

static uint64_t g_rd_system_off_prep_sequence(void)
{
    val_host_rmifeatureregister0_ts features_0;
    uint64_t ret;

    val_memset(&realm_test[SYSTEM_OFF_REALM], 0, sizeof(realm_test[SYSTEM_OFF_REALM]));
    val_memset(&features_0, 0, sizeof(features_0));
    features_0.s2sz = 40;
    val_memcpy(&realm_test[SYSTEM_OFF_REALM].realm_feat_0, &features_0, sizeof(features_0));

    realm_test[SYSTEM_OFF_REALM].hash_algo = RMI_HASH_SHA_256;
    realm_test[SYSTEM_OFF_REALM].s2_starting_level = 0;
    realm_test[SYSTEM_OFF_REALM].num_s2_sl_rtts = 1;
    realm_test[SYSTEM_OFF_REALM].vmid = SYSTEM_OFF_REALM;
    realm_test[SYSTEM_OFF_REALM].rec_count = 1;


    /* Populate realm with one REC */
    if (val_host_realm_setup(&realm_test[SYSTEM_OFF_REALM], 1)) {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Enter REC[0] */
    ret = val_host_rmi_rec_enter(realm_test[SYSTEM_OFF_REALM].rec[0],
                                 realm_test[SYSTEM_OFF_REALM].run[0]);
    if (ret) {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm_test[SYSTEM_OFF_REALM].rd;

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

    if (val_host_rec_create_common(&realm, &rec_params))
    {
        LOG(ERROR, "\tCouldn't destroy the Realm\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm.rec[0];
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

        case RD_DEV_MEM:
            args->rd = g_dev_mem_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_OUTSIDE_OF_PERMITTED_PA:
            args->rd = g_outside_of_permitted_pa_prep_sequence();
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
            args->rd = g_rec_ready_prep_sequence(c_args.rd_valid);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_STATE_RTT:
            args->rd = realm_test[ACTIVE_REALM].rtt_l0_addr;
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

        case LEVEL_OOB:
            args->rd = c_args.rd_valid;
            // The RTT hierarchy is created for this IPA
            // Note: we require the RTT walk to succeed here
            args->ipa = c_args.ipa_valid;
            args->level = level_invalid_oob_prep_sequence();
            break;

        case IPA_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->ipa = g_unaligned_prep_sequence(c_args.ipa_valid);
            args->level = c_args.level_valid;
            break;

        case IPA_OOB:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_outside_of_permitted_ipa_prep_sequence();
            args->level = c_args.level_valid;
            break;

        case REALM_ACTIVE:
            args->rd = g_rd_active_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case REALM_NULL:
            args->rd = g_rd_null_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case REALM_SYSTEM_OFF:
            args->rd = g_rd_system_off_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case IPA_NOT_MAPPED:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_protected_unmapped_prep_sequence();
            args->level = c_args.level_valid;
            break;

        case RTTE_STATE_TABLE:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->level = WALK_LEVEL_TABLE;
            break;

        case RTTE_STATE_ASSIGNED:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_protected_assigned_ram_prep_sequence(c_args.rd_valid);
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->level = c_args.level_valid;
            break;

        case RTTE_STATE_DESTROYED:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_protected_destroyed_empty_prep_sequence(c_args.rd_valid);
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->level = c_args.level_valid;
            break;

        default:
            /* set status to failure */
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }
    return VAL_SUCCESS;
}


void cmd_rtt_init_ripas_host(void)
{
    uint64_t ret;
    struct arguments args;
    val_host_rtt_entry_ts rtte;
    uint64_t i;

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto fail;
    }

    /* Iterate over the input */
    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "\n\tCheck %d : ", i + 1, 0);
        LOG(TEST, test_data[i].msg, 0, 0);
        LOG(TEST, "; intent id : 0x%x \n", test_data[i].label, 0);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto fail;
        }

        ret = val_host_rmi_rtt_init_ripas(args.rd, args.ipa, args.level);

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto fail;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);
    ret = val_host_rmi_rtt_init_ripas(c_args.rd_valid, c_args.ipa_valid,
                                c_args.level_valid);
    /* Valid call should give success if footprints have not changed */
    if (ret != 0) {
        LOG(ERROR, "\n\t Positive Observability Failed", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto fail;
    }

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid,
                                      c_args.ipa_valid, c_args.level_valid, &rtte);
    if (ret) {
        LOG(ERROR, "\n\tRead entry failed \n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto fail;
    }

    if (rtte.walk_level == c_args.level_valid && rtte.ripas != RMI_RAM) {
        LOG(ERROR, "\n\t Positive Observability Failed", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto fail;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));
    return;

fail:
    return;
}
