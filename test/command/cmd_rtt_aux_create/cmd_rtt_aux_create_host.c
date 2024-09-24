/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_rtt_aux_create_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512 * L3_SIZE)
#define L1_SIZE (512 * L2_SIZE)

#define NUM_REALMS            2
#define VALID_REALM           0
#define SINGLE_RTT_TREE_REALM 1

#define IPA_ADDR_PROTECTED_UNMAPPED L1_SIZE
#define IPA_ADDR_DATA  4 * PAGE_SIZE
#define IPA_ADDR_ZERO_PAGE     0
#define LEVEL_L1               1
#define LEVEL_L2               2
#define RTT_INDEX_1            1
#define RTT_INDEX_PRIMARY      0
#define NUM_AUX_PLANES         1
#define RTT_INDEX_OUT_OF_BOUND NUM_AUX_PLANES + 1
#define LEVEL_STARTING_LEVEL   0
#define LEVEL_OUT_OF_BOUND     4

static val_host_realm_ts realm[NUM_REALMS];
uint64_t data_gran;

static struct argument_store {
    uint64_t rd_valid;
    uint64_t rtt_valid;
    uint64_t ipa_valid;
    uint64_t level_valid;
    uint64_t index_valid;
} c_args;

struct arguments {
    uint64_t rd;
    uint64_t rtt;
    uint64_t ipa;
    uint64_t level;
    uint64_t index;
};

static uint64_t ipa_l1_unaligned_prep_sequence(void)
{
    return PAGE_SIZE;
}

static uint64_t g_lpa2_pa_prep_sequence(void)
{
    return 1ULL << 52;
}

static uint64_t rd_rtt_tree_single_prep_sequence(void)
{
    val_host_realm_flags1_ts realm_flags;
    uint64_t featreg0;

    val_memset(&realm[SINGLE_RTT_TREE_REALM], 0, sizeof(realm[SINGLE_RTT_TREE_REALM]));
    val_memset(&realm_flags, 0, sizeof(realm_flags));

    val_host_rmi_features(0, &featreg0);

    /* SKIP thie test case if RMM do not support both single and shared RTT configuration */
    if (VAL_EXTRACT_BITS(featreg0, 43, 44) != RMI_PLANE_RTT_AUX_SINGLE)
    {
        LOG(ALWAYS, "\n\tRMM does not support shared RTT tree configuration\n", 0, 0);
        return VAL_SKIP_CHECK;
    }

    val_host_realm_params(&realm[SINGLE_RTT_TREE_REALM]);

    realm[SINGLE_RTT_TREE_REALM].s2sz = IPA_WIDTH;
    realm[SINGLE_RTT_TREE_REALM].s2_starting_level = 0;
    realm[SINGLE_RTT_TREE_REALM].vmid = 2;
    realm[SINGLE_RTT_TREE_REALM].num_aux_planes = NUM_AUX_PLANES;
    realm_flags.rtt_tree_pp = RMI_FEATURE_FALSE;

    val_memcpy(&realm[SINGLE_RTT_TREE_REALM].flags1, &realm_flags,
                                         sizeof(realm[SINGLE_RTT_TREE_REALM].flags1));

    if (val_host_realm_create_common(&realm[SINGLE_RTT_TREE_REALM]))
    {
        LOG(ERROR, "\tRealm create failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm[SINGLE_RTT_TREE_REALM].rd;
}

static uint64_t rd_valid_prep_sequence(void)
{
    val_host_realm_flags1_ts realm_flags;

    val_memset(&realm[VALID_REALM], 0, sizeof(realm[VALID_REALM]));
    val_memset(&realm_flags, 0, sizeof(realm_flags));

    val_host_realm_params(&realm[VALID_REALM]);

    realm[VALID_REALM].s2sz = IPA_WIDTH;
    realm[VALID_REALM].s2_starting_level = 0;
    realm[VALID_REALM].vmid = VALID_REALM;
    realm[VALID_REALM].rec_count = 1;
    realm[VALID_REALM].num_aux_planes = 1;
    realm_flags.rtt_tree_pp = RMI_FEATURE_TRUE;

    val_memcpy(&realm[VALID_REALM].flags1, &realm_flags, sizeof(realm[VALID_REALM].flags1));

    if (val_host_realm_create_common(&realm[VALID_REALM]))
    {
        LOG(ERROR, "\tRealm create failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Create first branches of RTTs */
    if (val_host_create_aux_mapping(realm[VALID_REALM].rd, 0, RTT_INDEX_1))
    {
        LOG(ERROR, "\tRTT Creation failed\n", 0, 0)
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm[VALID_REALM].rd;
}

static uint64_t rtt_valid_prep_sequence(void)
{
    return g_delegated_prep_sequence();
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.rd_valid = rd_valid_prep_sequence();
    if (c_args.rd_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.rtt_valid = rtt_valid_prep_sequence();
    if (c_args.rtt_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.ipa_valid = IPA_ADDR_PROTECTED_UNMAPPED;

    c_args.level_valid = LEVEL_L2 ;

    c_args.index_valid = RTT_INDEX_1;

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
            args->index = c_args.index_valid;
            break;

        case RD_DEV_MEM:
            args->rd = g_dev_mem_prep_sequence();
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RD_OUTSIDE_OF_PERMITTED_PA:
            args->rd = g_outside_of_permitted_pa_prep_sequence();
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RD_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RD_STATE_DELEGATED:
            args->rd = g_delegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RD_STATE_REC:
            args->rd = realm[VALID_REALM].rec[0];
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RD_STATE_RTT:
            args->rd = realm[VALID_REALM].rtt_l0_addr;
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RD_STATE_DATA:
            args->rd = g_data_prep_sequence(c_args.rd_valid, IPA_ADDR_DATA);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            data_gran = args->rd;
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case LEVEL_STARTING:
            args->rd = c_args.rd_valid;
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = LEVEL_STARTING_LEVEL;
            args->index = c_args.index_valid;
            break;

        case LEVEL_OOB:
            args->rd = c_args.rd_valid;
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = LEVEL_OUT_OF_BOUND;
            args->index = c_args.index_valid;
            break;

        case IPA_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rtt = c_args.rtt_valid;
            args->ipa = ipa_l1_unaligned_prep_sequence();
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case IPA_OOB:
            args->rd = c_args.rd_valid;
            args->rtt = c_args.rtt_valid;
            args->ipa = ipa_outside_of_permitted_ipa_prep_sequence();
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case REALM_SINGLE_RTT_TREE:
            args->rd = rd_rtt_tree_single_prep_sequence();;
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            else if (args->rd == VAL_SKIP_CHECK)
                return VAL_SKIP_CHECK;
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case INDEX_PRIMARY_RTT_TREE:
            args->rd = c_args.rd_valid;
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = RTT_INDEX_PRIMARY;
            break;

        case INDEX_OUT_OF_BOUND:
            args->rd = c_args.rd_valid;
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = RTT_INDEX_OUT_OF_BOUND;
            break;

        case RTT_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rtt = g_unaligned_prep_sequence(c_args.rtt_valid);
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RTT_DEV_MEM:
            args->rd = c_args.rd_valid;
            args->rtt = g_dev_mem_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RTT_OUTSIDE_PERMITTED_PA:
            args->rd = c_args.rd_valid;
            args->rtt = g_outside_of_permitted_pa_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RTT_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->rtt = g_undelegated_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RTT_STATE_RD:
            args->rd = c_args.rd_valid;
            args->rtt = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RTT_STATE_REC:
            args->rd = c_args.rd_valid;
            args->rtt = realm[VALID_REALM].rec[0];
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RTT_STATE_RTT:
            args->rd = c_args.rd_valid;
            args->rtt = realm[VALID_REALM].rtt_l0_addr;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RTT_STATE_DATA:
            args->rd = c_args.rd_valid;
            args->rtt = data_gran;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RTT_LPA2_DISABLED:
            args->rd = c_args.rd_valid;
            args->rtt = g_lpa2_pa_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case LEVEL_NO_PARENT_RTTE:
            args->rd = c_args.rd_valid;
            args->rtt = c_args.rtt_valid;
            args->ipa = c_args.ipa_valid;
            /* ipa_valid is only mapped till L1 and trying to create a L3 table at this
             * address should give us walk fault */
            args->level = VAL_RTT_MAX_LEVEL;
            args->index = c_args.index_valid;
            break;

        case RTTE_STATE_TABLE:
            args->rd = c_args.rd_valid;
            args->rtt = c_args.rtt_valid;
            /* IPA addr 0x0000 is mapped till L3 and should contain a table entry at L1 */
            args->ipa = IPA_ADDR_ZERO_PAGE;
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        default:
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_rtt_aux_create_host(void)
{
    uint64_t i, ret, rtt;
    val_smc_param_ts cmd_ret;
    struct arguments args;

    /* Skip if RMM do not support planes */
    if (!val_host_rmm_supports_planes())
    {
        LOG(ALWAYS, "\n\tPlanes feature not supported\n", 0, 0);
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto exit;
    }

    /* Skip the test if implementation does not support RTT tree per plane*/
    if (!val_host_rmm_supports_rtt_tree_per_plane())
    {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        LOG(ALWAYS, "\tNo support for RTT tree per plane, Skipping Test\n", 0, 0);
        goto exit;
    }

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "\n\tCheck %d : ", i + 1, 0);
        LOG(TEST, test_data[i].msg, 0, 0);
        LOG(TEST, "; intent id : 0x%x \n", test_data[i].label, 0);

        ret = intent_to_seq(&test_data[i], &args);
        if (ret == VAL_SKIP_CHECK)
        {
            LOG(TEST, "\tSkipping Check %d\n", i + 1, 0);
            continue;
        }
        else if (ret == VAL_ERROR) {
            LOG(ERROR, "\n\t Intent to sequence failed \n", 0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        cmd_ret = val_host_rmi_rtt_aux_create(args.rd, args.rtt, args.ipa, args.level, args.index);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);
    cmd_ret = val_host_rmi_rtt_aux_create(c_args.rd_valid, c_args.rtt_valid, c_args.ipa_valid,
                                                          c_args.level_valid, c_args.index_valid);
    if (cmd_ret.x0 != 0)
    {
        LOG(ERROR, "\n\t RTT_AUX_CREATE failed with ret = %x\n", cmd_ret.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    /* Calling RTT_AUX_CREATE again should fail with RMI_ERROR_INPUT due to granule state
     * transition of RTT granule*/
    cmd_ret = val_host_rmi_rtt_aux_create(c_args.rd_valid, c_args.rtt_valid, c_args.ipa_valid,
                                                          c_args.level_valid, c_args.index_valid);
    if (cmd_ret.x0 != RMI_ERROR_INPUT)
    {
        LOG(ERROR, "\n\t unexpected output for RTT_AUX_CREATE ret = %x\n", cmd_ret.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    rtt = g_delegated_prep_sequence();
    if (rtt == VAL_TEST_PREP_SEQ_FAILED)
    {
        LOG(ERROR, "\n\t RTT granule delegation failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    /* Calling RTT_AUX_CREATE again with a valid RTT granule should fail with RMI_ERROR_RTT_AUX
     * as RTTE state of the parent entry is transitioned to TABLE*/
    cmd_ret = val_host_rmi_rtt_aux_create(c_args.rd_valid, rtt, c_args.ipa_valid,
                                                          c_args.level_valid, c_args.index_valid);
    if (RMI_STATUS(cmd_ret.x0) != RMI_ERROR_RTT_AUX || RMI_INDEX(cmd_ret.x0) != LEVEL_L1)
    {
        LOG(ERROR, "\n\t unexpected output for RTT_AUX_CREATE ret = %x\n", cmd_ret.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
