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
    uint64_t base_valid;
    uint64_t top_valid;
} c_args;

struct arguments {
    uint64_t rd;
    uint64_t base;
    uint64_t top;
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
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    realm_test[vmid].rd = realm_init.rd;
    realm_test[vmid].rtt_l0_addr = realm_init.rtt_l0_addr;
    return realm_init.rd;
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
    uint64_t ret;

    val_memset(&realm_test[SYSTEM_OFF_REALM], 0, sizeof(realm_test[SYSTEM_OFF_REALM]));

    val_host_realm_params(&realm_test[SYSTEM_OFF_REALM]);

    realm_test[SYSTEM_OFF_REALM].vmid = SYSTEM_OFF_REALM;

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
    uint64_t rd, ret;
    rd = g_rd_new_prep_sequence(NULL_REALM);
    if (rd == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    ret = val_host_rmi_realm_destroy(rd);
    if (ret)
    {
        LOG(ERROR, "\tCouldn't destroy the Realm, ret: %d\n", ret, 0);
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

static uint64_t rd_valid_prep_sequence(void)
{
    return g_rd_new_prep_sequence(VALID_REALM);
}

static uint64_t base_valid_prep_sequence(void)
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

/* top_align requires top to greater than or equal to next level aligned IPA after base
 * Since we create mapping till L3, top >= base + L3_SIZE */
static uint64_t top_valid_prep_sequence(void)
{
    return IPA_ADDR_VALID + L3_SIZE;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.rd_valid = rd_valid_prep_sequence();
    if (c_args.rd_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.base_valid = base_valid_prep_sequence();
    if (c_args.base_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.top_valid = top_valid_prep_sequence();

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_DEV_MEM:
            args->rd = g_dev_mem_prep_sequence();
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_OUTSIDE_OF_PERMITTED_PA:
            args->rd = g_outside_of_permitted_pa_prep_sequence();
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_STATE_DELEGATED:
            args->rd = g_delegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_STATE_REC:
            args->rd = g_rec_ready_prep_sequence(c_args.rd_valid);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_STATE_RTT:
            args->rd = realm_test[ACTIVE_REALM].rtt_l0_addr;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_STATE_DATA:
            args->rd = g_data_prep_sequence(c_args.rd_valid, IPA_ADDR_DATA);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case SIZE_INVALID:
            args->rd = c_args.rd_valid;
            args->base = c_args.top_valid;
            args->top = c_args.base_valid;
            break;

        case TOP_UNPROTECTED:
            args->rd = c_args.rd_valid;
            args->base = c_args.base_valid;
            args->top = ipa_unprotected_unassigned_prep_sequence(c_args.rd_valid) + PAGE_SIZE;
            break;

        case REALM_ACTIVE:
            args->rd = g_rd_active_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REALM_SYSTEM_OFF:
            args->rd = g_rd_system_off_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REALM_NULL:
            args->rd = g_rd_null_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case BASE_LEVEL_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->base = ipa_protected_unmapped_prep_sequence() + PAGE_SIZE;
            args->top = args->base + L2_SIZE;
            break;

        case RTTE_STATE_ASSIGNED:
            args->rd = c_args.rd_valid;
            args->base = ipa_protected_assigned_ram_prep_sequence(c_args.rd_valid);
            if (args->base == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->top = args->base + L3_SIZE;
            break;

        case TOP_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->base = ipa_protected_unmapped_prep_sequence();
            args->top = args->base + L3_SIZE;
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
    uint64_t out_top;

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

        ret = val_host_rmi_rtt_init_ripas(args.rd, args.base, args.top, &out_top);

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);
    ret = val_host_rmi_rtt_init_ripas(c_args.rd_valid, c_args.base_valid,
                                c_args.top_valid, &out_top);
    /* Valid call should give success if footprints have not changed */
    if (ret != 0) {
        LOG(ERROR, "\n\t Positive Observability Failed", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid,
                                      c_args.base_valid, MAP_LEVEL, &rtte);
    if (ret) {
        LOG(ERROR, "\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    if (rtte.walk_level == MAP_LEVEL && rtte.ripas != RMI_RAM) {
        LOG(ERROR, "\n\t Unexpected RTT entry. walk level = %d, RIPAS = %d",
                                                 rtte.walk_level, rtte.ripas);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
