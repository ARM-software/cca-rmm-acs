/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
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

/* Keep the VOID success range separate from the fixed DATA-entry test IPA. */

#define IPA_ADDR_VALID (8 * PAGE_SIZE)
#define IPA_ADDR_DATA  (16 * PAGE_SIZE)

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
        LOG(ERROR, "Realm create failed\n");
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
        LOG(ERROR, "Couldn't activate the Realm\n");
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
        LOG(ERROR, "Realm setup failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Enter REC[0] */
    ret = val_host_rmi_rec_enter(realm_test[SYSTEM_OFF_REALM].rec[0],
                                 realm_test[SYSTEM_OFF_REALM].run[0]);
    if (ret) {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
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

    ret = val_host_rmi_realm_terminate(rd);
    if (ret)
    {
        LOG(ERROR, "Couldn't terminate the Realm, ret: %d\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    ret = val_host_rmi_realm_destroy(rd);
    if (ret)
    {
        LOG(ERROR, "Couldn't destroy the Realm, ret: %d\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    return rd;
}

static uint64_t g_rec_ready_prep_sequence(uint64_t rd)
{
    val_host_realm_ts realm = {0};
    val_host_rec_params_ts rec_params;

    realm.rec_count = 1;
    realm.rd = rd;
    rec_params.pc = 0;
    rec_params.flags = RMI_RUNNABLE;

    if (val_host_rec_create_common(&realm, &rec_params))
    {
        LOG(ERROR, "Couldn't destroy the Realm\n");
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
        LOG(ERROR, "Couldn't create the protected mapping\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return IPA_ADDR_VALID;
}

/* Cover two adjacent leaf entries in the successful VOID range. */
static uint64_t top_valid_prep_sequence(void)
{
    return IPA_ADDR_VALID + (2 * L3_SIZE);
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

static uint64_t no_progress_walk_prep_sequence(uint64_t base, uint64_t top)
{
    val_host_rtt_entry_ts rtte;

    /* A granule-sized range cannot consume the level-2 entry at base. */
    if (((base % L2_SIZE) != 0U) || (top <= base) ||
        ((top - base) >= L2_SIZE))
    {
        LOG(ERROR, "No-progress range does not end inside one level-2 entry\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (val_host_rmi_rtt_read_entry(c_args.rd_valid, base, MAP_LEVEL, &rtte) ||
        (rtte.walk_level != 2) || (rtte.state != RMI_RTTE_VOID))
    {
        LOG(ERROR, "No-progress setup did not reach a level-2 VOID entry\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

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
            args->rd = realm_test[VALID_REALM].rtt_l0_addr;
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

        case TOP_GRAN_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->base = c_args.base_valid;
            args->top = L3_SIZE / 2;
            break;

        case NO_PROGRESS_SHORT_RANGE:
            args->rd = c_args.rd_valid;
            args->base = ipa_protected_unmapped_prep_sequence();
            args->top = args->base + L3_SIZE;
            if (((args->top % PAGE_SIZE) != 0U) ||
                no_progress_walk_prep_sequence(args->base, args->top))
                return VAL_TEST_PREP_SEQ_FAILED;
            break;

        case TOP_GRAN_UNALIGNED_NO_PROGRESS:
            args->rd = c_args.rd_valid;
            args->base = ipa_protected_unmapped_prep_sequence();
            args->top = args->base + (L3_SIZE / 2);
            if (((args->top % PAGE_SIZE) == 0U) ||
                no_progress_walk_prep_sequence(args->base, args->top))
                return VAL_TEST_PREP_SEQ_FAILED;
            break;

        default:
            /* set status to failure */
            LOG(ERROR, "Unknown intent label encountered\n");
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
    uint64_t data_ipa;
    uint64_t next_ipa;

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    /* Iterate over the input */
    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
              i + 1, test_data[i].msg, test_data[i].label);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        ret = val_host_rmi_rtt_init_ripas_smc(args.rd, args.base, args.top, &out_top).x0;

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "Test Failure!The ABI call returned: %xExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);
    next_ipa = c_args.base_valid;
    while (next_ipa < c_args.top_valid)
    {
        ret = val_host_rmi_rtt_init_ripas(c_args.rd_valid, next_ipa,
                                           c_args.top_valid, &out_top);
        if (ret != RMI_SUCCESS)
        {
            LOG(ERROR, "VOID range RIPAS initialization failed, ret=0x%lx\n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }

        if ((out_top <= next_ipa) || (out_top > c_args.top_valid))
        {
            LOG(ERROR, "Invalid out_top received: 0x%lx\n", out_top);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto exit;
        }

        next_ipa = out_top;
    }

    for (i = c_args.base_valid; i < c_args.top_valid; i += L3_SIZE)
    {
        ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, i, MAP_LEVEL, &rtte);
        if (ret || (rtte.walk_level != MAP_LEVEL) ||
            (rtte.state != RMI_RTTE_VOID) || (rtte.ripas != RMI_RIPAS_RAM))
        {
            LOG(ERROR, "VOID entry at IPA 0x%lx was not initialized to RAM\n", i);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
            goto exit;
        }
    }

    data_ipa = ipa_protected_assigned_empty_prep_sequence(c_args.rd_valid);
    if (data_ipa == VAL_TEST_PREP_SEQ_FAILED)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, data_ipa, MAP_LEVEL, &rtte);
    if (ret || (rtte.walk_level != MAP_LEVEL) ||
        (rtte.state != RMI_RTTE_DATA) || (rtte.ripas != RMI_RIPAS_EMPTY))
    {
        LOG(ERROR, "DATA entry was not prepared with RIPAS_EMPTY\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    ret = val_host_rmi_rtt_init_ripas(c_args.rd_valid, data_ipa,
                                       data_ipa + L3_SIZE, &out_top);
    if ((ret != RMI_SUCCESS) || (out_top != data_ipa + L3_SIZE))
    {
        LOG(ERROR, "DATA entry RIPAS initialization failed, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, data_ipa, MAP_LEVEL, &rtte);
    if (ret || (rtte.walk_level != MAP_LEVEL) ||
        (rtte.state != RMI_RTTE_DATA) || (rtte.ripas != RMI_RIPAS_RAM))
    {
        LOG(ERROR, "DATA entry was not initialized to RAM\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
