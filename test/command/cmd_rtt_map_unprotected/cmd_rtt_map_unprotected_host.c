/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_rtt_map_unprotected_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define START_RTT_LEVEL 0
#define MAX_RTT_LEVEL 3
#define MAX_GRANULES 256

#define MAP_LEVEL 3
#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512L * L3_SIZE)
#define L1_SIZE (512L * L2_SIZE)
#define IPA_ADDR_DATA  (4 * PAGE_SIZE)

/*      Valid Realm memory layout
 *
 * ipa: 0x0 (1st protected L3 Entry)
 *  ------------------------------------
 * |  HIPAS = UNASSIGNED, RIPAS = RAM   |
 *  ------------------------------------
 * ipa: 0x1000
 *
 *                  (...)
 *
 *
 * ipa: 0x1XXX000 (1st unprotected L3 Entry)
 *  ------------------------------------
 * |         HIPAS = VALID_NS           |
 *  ------------------------------------
 * |        HIPAS = UNASSIGNED          |
 *  ------------------------------------
 * ipa: 0x1XX2000
 *
 */

#define IPA_ADDR_UNPROTECTED_UNASSIGNED (1UL << (IPA_WIDTH - 1)) + L3_SIZE

#define NUM_REALMS 1
#define VALID_REALM 0

static val_host_realm_ts realm_test[NUM_REALMS];

static struct argument_store {
    uint64_t rd_valid;
    uint64_t ipa_valid;
    uint64_t level_valid;
    uint64_t desc_valid;
} c_args;

struct arguments {
    uint64_t rd;
    uint64_t ipa;
    uint64_t level;
    uint64_t desc;
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

static uint64_t rd_valid_prep_sequence(void)
{
    return g_rd_new_prep_sequence(VALID_REALM);
}

static uint64_t ipa_valid_prep_sequence(void)
{
    /* RTTE[ipa].state = UNASSIGNED */

    if (create_mapping(IPA_ADDR_UNPROTECTED_UNASSIGNED, false, c_args.rd_valid))
    {
        LOG(ERROR, "\tCouldn't create mapping\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    return IPA_ADDR_UNPROTECTED_UNASSIGNED;
}


static uint64_t level_valid_prep_sequence(void)
{
    return MAP_LEVEL;
}


static uint64_t desc_valid_prep_sequence(void)
{
    uint64_t ns = g_undelegated_prep_sequence();
    if (ns == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    return (ns | ATTR_NORMAL_WB_WA_RA | ATTR_STAGE2_AP_RW | ATTR_INNER_SHARED);
}


static uint64_t mem_attr_invalid_prep_sequence(uint64_t attr_valid)
{
    return (attr_valid | 1L << 52);
}

static uint64_t level_invalid_starting_prep_sequence(void)
{
    return START_RTT_LEVEL;
}


static uint64_t level_invalid_l1_prep_sequence(void)
{
    return 1;
}


static uint64_t level_invalid_oob_prep_sequence(void)
{
    return MAX_RTT_LEVEL + 1;
}


static uint64_t desc_addr_unaligned_prep_sequence(void)
{
    uint64_t ns = g_undelegated_prep_sequence();
    if (ns == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    ns = ns + (L3_SIZE / 2);

    return (ns | ATTR_NORMAL_WB_WA_RA | ATTR_STAGE2_AP_RW | ATTR_INNER_SHARED);
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
        LOG(ERROR, "\tREC create failed\n", 0, 0);
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

    c_args.desc_valid = desc_valid_prep_sequence();
    if (c_args.desc_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case MEM_ATTR_INVALID:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->desc = mem_attr_invalid_prep_sequence(c_args.desc_valid);
            break;

        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->desc = c_args.desc_valid;
            break;

        case RD_DEV_MEM:
            args->rd = g_dev_mem_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->desc = c_args.desc_valid;
            break;

	case RD_OUTSIDE_OF_PERMITTED_PA:
            args->rd = g_outside_of_permitted_pa_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->desc = c_args.desc_valid;
            break;

        case RD_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->desc = c_args.desc_valid;
            break;

        case RD_STATE_DELEGATED:
            args->rd = g_delegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->desc = c_args.desc_valid;
            break;

	case RD_STATE_REC:
            args->rd = g_rec_ready_prep_sequence(c_args.rd_valid);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->desc = c_args.desc_valid;
            break;

	case RD_STATE_RTT:
            args->rd = realm_test[VALID_REALM].rtt_l0_addr;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->desc = c_args.desc_valid;
            break;

	case RD_STATE_DATA:
            args->rd = g_data_prep_sequence(c_args.rd_valid, IPA_ADDR_DATA);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->desc = c_args.desc_valid;
            break;

        case LEVEL_STARTING:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = level_invalid_starting_prep_sequence();
            args->desc = c_args.desc_valid;
            break;

        case LEVEL_L1:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = level_invalid_l1_prep_sequence();
            args->desc = c_args.desc_valid;
            break;

        case LEVEL_OOB:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = level_invalid_oob_prep_sequence();
            args->desc = c_args.desc_valid;
            break;

        case ADDR_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->desc = desc_addr_unaligned_prep_sequence();
            break;

        case IPA_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->ipa = g_unaligned_prep_sequence(c_args.ipa_valid);
            args->level = c_args.level_valid;
            args->desc = c_args.desc_valid;
            break;

        case IPA_PROTECTED:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_protected_unassigned_empty_prep_sequence(c_args.rd_valid);
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->level = c_args.level_valid;
            args->desc = c_args.desc_valid;
            break;

        case IPA_OOB:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_outside_of_permitted_ipa_prep_sequence();
            args->level = c_args.level_valid;
            args->desc = c_args.desc_valid;
            break;

        case IPA_NOT_MAPPED:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_unprotected_unmapped_prep_sequence();
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->level = c_args.level_valid;
            args->desc = c_args.desc_valid;
            break;

        case RTTE_STATE_ASSIGNED_NS:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_unprotected_assinged_prep_sequence(c_args.rd_valid);
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->level = c_args.level_valid;
            args->desc = c_args.desc_valid;
            break;

        default:
            /* set status to failure */
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }
    return VAL_SUCCESS;
}


void cmd_rtt_map_unprotected_host(void)
{
    uint64_t ret;
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

        ret = val_host_rmi_rtt_map_unprotected(args.rd, args.ipa, args.level, args.desc);

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    /* Check that rtte.addr and rtte.state have not changed */
    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                      c_args.level_valid, &rtte);
    if (ret) {
        LOG(ERROR, "\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    } else {
        if (rtte.state != RMI_UNASSIGNED || rtte.desc == c_args.desc_valid) {
            LOG(ERROR, "\tUnexpected RTT entry.\n", 0, 0);
            LOG(ERROR, "\tState is: %d & desc is: %x\n", rtte.state, rtte.desc);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);
    ret = val_host_rmi_rtt_map_unprotected(c_args.rd_valid, c_args.ipa_valid,
                                      c_args.level_valid, c_args.desc_valid);
    /* Valid call should give success if footprints have not changed */
    if (ret != 0) {
        LOG(ERROR, "\n\tPositive Observability Check failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    /* Check that rtte.addr and rtte.state have changed */
    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                      c_args.level_valid, &rtte);
    if (ret) {
        LOG(ERROR, "\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    } else {
        if (rtte.state != RMI_VALIDN_NS || rtte.desc != c_args.desc_valid) {
            LOG(ERROR, "\tUnexpected RTT entry.\n", 0, 0);
            LOG(ERROR, "\tState is: %d & desc is: %x\n", rtte.state, rtte.desc);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tNegative Observability Check\n", 0, 0);
    ret = val_host_rmi_rtt_map_unprotected(c_args.rd_valid, c_args.ipa_valid,
                                      c_args.level_valid, c_args.desc_valid);
    /* Now the command should fail with the same parameters (footprint has changed) */
    if (ret == 0) {
        LOG(ERROR, "\n\t Negative Observablity Check failed", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
