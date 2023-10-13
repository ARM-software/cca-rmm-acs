/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_rtt_read_entry_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define START_RTT_LEVEL 0
#define MAX_GRANULES 256

#define MAP_LEVEL 1
#define WALK_ERR_LEVEL 2
#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512L * L3_SIZE)
#define L1_SIZE (512L * L2_SIZE)
#define L0_SIZE (512L * L1_SIZE)
#define IPA_ADDR_DATA  (4 * PAGE_SIZE)

/*      Valid Realm memory layout
 *
 * ipa: 0x0 (1st <START_LEVEL> Table Entry)
 *  ------------------------------------
 * |        RTTE.state = TABLE          | -> mappings created up to leaf level
 *  ------------------------------------
 * ipa: SL_SIZE
 *
 */

#define IPA_ADDR_MAPPED 0x0
#define IPA_ADDR_UNPROTECTED (1UL << (IPA_WIDTH - 1))

#define NUM_REALMS 1
#define VALID_REALM 0

static val_host_realm_ts realm_test[NUM_REALMS];

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
    /* RTTE[ipa].state = UNASSIGNED
       RTTE[ipa].ripas = EMPTY */

    if (create_mapping(IPA_ADDR_MAPPED, false, c_args.rd_valid))
    {
        LOG(ERROR, "\tCouldn't create the protected mapping\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    return IPA_ADDR_MAPPED;
}


static uint64_t level_valid_prep_sequence(void)
{
    return VAL_RTT_MAX_LEVEL;
}

static uint64_t level_invalid_oob_prep_sequence(void)
{
    return VAL_RTT_MAX_LEVEL + 1;
}

static uint64_t g_rec_ready_prep_sequence(uint64_t rd)
{
    val_host_realm_ts realm;
    val_host_rec_params_ts rec_params;

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
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED) {
                LOG(ERROR, "\tThe undelegated preparation sequence failed\n", 0, 0);
                return VAL_ERROR;
            }
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_STATE_DELEGATED:
            args->rd = g_delegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED) {
                LOG(ERROR, "\tThe delegated preparation sequence failed\n", 0, 0);
                return VAL_ERROR;
            }
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_STATE_REC:
            args->rd = g_rec_ready_prep_sequence(c_args.rd_valid);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED) {
                LOG(ERROR, "\tREC preparation sequence failed\n", 0, 0);
                return VAL_ERROR;
            }
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_STATE_RTT:
            args->rd = realm_test[VALID_REALM].rtt_l0_addr;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            break;

        case RD_STATE_DATA:
            args->rd = g_data_prep_sequence(c_args.rd_valid, IPA_ADDR_DATA);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED) {
                LOG(ERROR, "\tREC preparation sequence failed\n", 0, 0);
                return VAL_ERROR;
            }
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
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

        case IPA_OOB:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_outside_of_permitted_ipa_prep_sequence();
            args->level = c_args.level_valid;
            break;

        default:
            /* set status to failure */
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }
    return VAL_SUCCESS;
}


void cmd_rtt_read_entry_host(void)
{
    uint64_t ret;
    struct arguments args;
    uint64_t i;
    val_host_rtt_entry_ts rtte;

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    // Iterate over the input
    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "\n\tCheck %d : ", i + 1, 0);
        LOG(TEST, test_data[i].msg, 0, 0);
        LOG(TEST, "; intent id : 0x%x \n", test_data[i].label, 0);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        ret = val_host_rmi_rtt_read_entry(args.rd, args.ipa, args.level, &rtte);

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tNegative Observability \n", 0, 0);
    /* Check when command failed X1-X4 is zero*/

    /* Initialize a zero filled reference structure and compare with the command output */
    val_host_rtt_entry_ts zero_ref = {0};
    if (val_memcmp(&rtte, &zero_ref, sizeof(rtte))) {
        LOG(ERROR, "\t Output arguments are not zero \n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    LOG(TEST, "\n\tPositive Observability \n", 0, 0);

    /* Check if walk level matches input level */
    for (i = 0; i < VAL_RTT_MAX_LEVEL; i++) {
        ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid, i, &rtte);
        if (ret) {
            LOG(ERROR, "\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto exit;
        }

        if (rtte.walk_level != i) {
            LOG(ERROR, "\n\tWalk level did not match \n", 0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
            goto exit;
        }
    }

    /* For the Valid IPA (UNASSIGNED & EMPTY) check for X2(state) and X4(ripas)
       and X(3) is zero */
    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                                       c_args.level_valid, &rtte);
    if (ret) {
        LOG(ERROR, "\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    if (rtte.state != RMI_UNASSIGNED || rtte.state != RMI_EMPTY || rtte.desc != 0) {
        LOG(ERROR, "\n\t Unexpected RTT entry.\n\tState is: %d, desc is: 0x%x",
                                                             rtte.state, rtte.desc);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    /* For protected IPA with ASSIGNED state check  if MemAttr, S2AP and SH are zeroes */
    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, IPA_ADDR_DATA,
                                                       c_args.level_valid, &rtte);
    if (ret) {
        LOG(ERROR, "\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

    if (rtte.state == RMI_ASSIGNED && VAL_EXTRACT_BITS(rtte.desc, 2, 9) != 0) {
        LOG(ERROR, "\n\t Unexpected descriptor Lower attributes  \n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto exit;
    }

    /* For unprotected IPA with Assigned state check for validity fo X3(desc) and X4(ripas) */
    if (create_mapping(IPA_ADDR_UNPROTECTED, false, c_args.rd_valid))
    {
        LOG(ERROR, "\tCouldn't create the unprotected mapping\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto exit;
    }

    uint64_t ns = g_undelegated_prep_sequence();
    if (ns == VAL_TEST_PREP_SEQ_FAILED) {
        LOG(ERROR, "\tUndelegated preparation sequence failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto exit;
    }

    uint64_t desc = (ns | ATTR_NORMAL_WB_WA_RA | ATTR_STAGE2_AP_RW | ATTR_INNER_SHARED);
    if (val_host_rmi_rtt_map_unprotected(c_args.rd_valid, IPA_ADDR_UNPROTECTED,
                                                         VAL_RTT_MAX_LEVEL, desc))
    {
        LOG(ERROR, "\tCouldn't complete the unprotected mapping\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto exit;
    }

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, IPA_ADDR_UNPROTECTED,
                                                       c_args.level_valid, &rtte);
    if (ret) {
        LOG(ERROR, "\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(14)));
        goto exit;
    }

    if (rtte.state == RMI_ASSIGNED && (rtte.desc != desc || rtte.ripas != RMI_EMPTY)) {
        LOG(ERROR, "\n\t Unexpected RTT entry\n\t State is: %d", rtte.state, 0);
        LOG(ERROR, " ,desc is: 0x%x & RIPAS is: %d\n", rtte.desc, rtte.ripas);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(15)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
