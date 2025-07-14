/*
 * Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_rtt_aux_fold_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512 * L3_SIZE)
#define L1_SIZE (512 * L2_SIZE)
#define L0_SIZE (512UL * L1_SIZE)

#define NUM_REALMS            2
#define VALID_REALM           0
#define SINGLE_RTT_TREE_REALM 1

#define IPA_ADDR_DATA               (5 * PAGE_SIZE)
#define IPA_NON_HOMOGENEOUS_RTT     0
#define IPA_HOMOGENEOUS_RTT         L2_SIZE
#define IPA_FOLDED_RTT              (5 * L2_SIZE)
#define IPA_UNASSIGNED              (6 * L2_SIZE)
#define IPA_UNMAPPED                L0_SIZE
#define RTT_INDEX_1                 1
#define RTT_INDEX_PRIMARY           0
#define NUM_AUX_PLANES              1
#define RTT_INDEX_OUT_OF_BOUND      NUM_AUX_PLANES + 1
#define MAP_LEVEL                   3
#define LEVEL_L2                    2
#define LEVEL_STARTING_LEVEL        0
#define LEVEL_INVALID               4

static val_host_realm_ts realm[NUM_REALMS];

static struct argument_store {
    uint64_t rd_valid;
    uint64_t ipa_valid;
    uint64_t level_valid;
    uint64_t index_valid;
} c_args;

struct arguments {
    uint64_t rd;
    uint64_t ipa;
    uint64_t level;
    uint64_t index;
};

static uint64_t ipa_non_homogeneous_rtt_prep_sequence(void)
{
    if (ipa_protected_aux_assigned_prep_sequence(realm[VALID_REALM].rd, RTT_INDEX_1)
                                                      == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    return IPA_NON_HOMOGENEOUS_RTT;
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
        LOG(ALWAYS, "RMM does not support shared RTT tree configuration\n");
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
        LOG(ERROR, "Realm create failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm[SINGLE_RTT_TREE_REALM].rd;
}

static uint64_t ipa_folded_rtt_prep_sequence(void)
{
    uint64_t phys;
    val_data_create_ts data_create;

    if (create_mapping(IPA_FOLDED_RTT, true, realm[VALID_REALM].rd))
    {
        LOG(ERROR, " Mapping creation failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (val_host_create_aux_mapping(realm[VALID_REALM].rd, IPA_FOLDED_RTT, RTT_INDEX_1))
    {
        LOG(ERROR, "RTT creation failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    phys = (uint64_t)val_host_mem_alloc(L2_SIZE, L2_SIZE);
    if (!phys)
    {
        LOG(ERROR, "val_host_mem_alloc failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    data_create.size = L2_SIZE;
    data_create.src_pa = phys;
    data_create.target_pa = phys + data_create.size;
    data_create.ipa = IPA_FOLDED_RTT;
    data_create.rtt_alignment = L2_SIZE;

    if (val_host_map_protected_data_to_realm(&realm[VALID_REALM], &data_create))
    {
        LOG(ERROR, "val_host_map_protected_data_to_realm failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (val_host_rmi_rtt_aux_fold(realm[VALID_REALM].rd, IPA_FOLDED_RTT, MAP_LEVEL, RTT_INDEX_1).x0)
    {
        LOG(ERROR, "RTT_AUX Fold failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return IPA_FOLDED_RTT;
}

static uint64_t ipa_valid_prep_sequence(void)
{
    uint64_t phys;
    val_data_create_ts data_create;

    if (create_mapping(IPA_HOMOGENEOUS_RTT, true, realm[VALID_REALM].rd))
    {
        LOG(ERROR, " Mapping creation failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (val_host_create_aux_mapping(realm[VALID_REALM].rd, IPA_HOMOGENEOUS_RTT, RTT_INDEX_1))
    {
        LOG(ERROR, "RTT Creation failed\n")
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    data_create.size = L2_SIZE;
    phys = (uint64_t)val_host_mem_alloc(L2_SIZE, (2 * data_create.size));
    if (!phys)
    {
        LOG(ERROR, "val_host_mem_alloc failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    data_create.src_pa = phys;
    data_create.target_pa = phys + data_create.size;
    data_create.ipa = IPA_HOMOGENEOUS_RTT;
    data_create.rtt_alignment = L2_SIZE;
    if (val_host_map_protected_data_to_realm(&realm[VALID_REALM], &data_create))
    {
        LOG(ERROR, "val_host_map_protected_data_to_realm failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return data_create.ipa;
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
    realm[VALID_REALM].num_aux_planes = NUM_AUX_PLANES;
    realm_flags.rtt_tree_pp = RMI_FEATURE_TRUE;

    val_memcpy(&realm[VALID_REALM].flags1, &realm_flags, sizeof(realm[VALID_REALM].flags1));

    if (val_host_realm_create_common(&realm[VALID_REALM]))
    {
        LOG(ERROR, "Realm create failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }


    return realm[VALID_REALM].rd;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.rd_valid = rd_valid_prep_sequence();
    if (c_args.rd_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.ipa_valid = ipa_valid_prep_sequence();
    if (c_args.ipa_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.level_valid = MAP_LEVEL;

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
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RD_OUTSIDE_OF_PERMITTED_PA:
            args->rd = g_outside_of_permitted_pa_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RD_DEV_MEM:
            args->rd = g_dev_mem_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RD_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RD_STATE_DELEGATED:
            args->rd = g_delegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RD_STATE_REC:
            args->rd = realm[VALID_REALM].rec[0];
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RD_STATE_RTT:
            args->rd = realm[VALID_REALM].rtt_l0_addr;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RD_STATE_DATA:
            args->rd = g_data_prep_sequence(c_args.rd_valid, IPA_ADDR_DATA);
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case LEVEL_STARTING_LEVLE:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = LEVEL_STARTING_LEVEL;
            args->index = c_args.index_valid;
            break;

        case LEVEL_OUT_OF_BOUND:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = LEVEL_INVALID;
            args->index = c_args.index_valid;
            break;

        case IPA_L2_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->ipa = PAGE_SIZE;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case IPA_OUT_OF_BOUND:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_outside_of_permitted_ipa_prep_sequence();
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case REALM_SINGLE_RTT_TREE:
            args->rd = rd_rtt_tree_single_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            else if (args->rd == VAL_SKIP_CHECK)
                return VAL_SKIP_CHECK;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case INDEX_PRIMARY_RTT_TREE:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = RTT_INDEX_PRIMARY;
            break;

        case INDEX_OUT_OF_BOUND:
            args->rd = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->index = RTT_INDEX_OUT_OF_BOUND;
            break;

        case IPA_NOT_MAPPED:
            args->rd = c_args.rd_valid;
            args->ipa = IPA_UNMAPPED;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RTTE_UNASSIGED:
            args->rd = c_args.rd_valid;
            args->ipa = IPA_UNASSIGNED;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RTTE_ASSIGNED:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_folded_rtt_prep_sequence();
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case RTT_NON_HOMOGENEOUS:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_non_homogeneous_rtt_prep_sequence();
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->level = c_args.level_valid;
            args->index = c_args.index_valid;
            break;

        case LEVEL_BOUND_RTT_WALK:
            args->rd = c_args.rd_valid;
            args->ipa = IPA_UNMAPPED;
            args->level = LEVEL_STARTING_LEVEL;
            args->index = c_args.index_valid;
            break;

        case LEVEL_BOUND_RTTE_STATE:
            args->rd = c_args.rd_valid;
            args->ipa = IPA_UNASSIGNED;
            args->level = LEVEL_STARTING_LEVEL;
            args->index = c_args.index_valid;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_rtt_aux_fold_host(void)
{
    uint64_t i, ret;
    val_smc_param_ts cmd_ret;
    struct arguments args;

    /* Skip if RMM do not support planes */
    if (!val_host_rmm_supports_planes())
    {
        LOG(ALWAYS, "Planes feature not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto exit;
    }

    /* Skip the test if implementation does not support RTT tree per plane*/
    if (!val_host_rmm_supports_rtt_tree_per_plane())
    {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        LOG(ALWAYS, "No support for RTT tree per plane, Skipping Test\n");
        goto exit;
    }

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
              i + 1, test_data[i].msg, test_data[i].label);

        ret = intent_to_seq(&test_data[i], &args);
        if (ret == VAL_SKIP_CHECK)
        {
            LOG(TEST, "Skipping Check %d\n", i + 1);
            continue;
        }
        else if (ret == VAL_ERROR) {
            LOG(ERROR, " Intent to sequence failed \n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }


        cmd_ret = val_host_rmi_rtt_aux_fold(args.rd, args.ipa, args.level, args.index);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "Test Failure!The ABI call returned: %xExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);
    cmd_ret = val_host_rmi_rtt_aux_fold(c_args.rd_valid, c_args.ipa_valid,
                                                 c_args.level_valid, c_args.index_valid);
    if (cmd_ret.x0 != 0)
    {
        LOG(ERROR, " Command failed. %x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    uint64_t undelegated_rtt = cmd_ret.x1;

    /* Executing RMI_RTT_AUX_FOLD AGAIN should fail with RMI_ERROR_RTT_AUX at L2 */
    cmd_ret = val_host_rmi_rtt_aux_fold(c_args.rd_valid, c_args.ipa_valid,
                                                 c_args.level_valid, c_args.index_valid);
    if (RMI_STATUS(cmd_ret.x0) != RMI_ERROR_RTT_AUX || RMI_INDEX(cmd_ret.x0) != LEVEL_L2)
    {
        LOG(ERROR, "Unexpected output for RTT_AUX_DESTROY %x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    /* Successful execution of RTT_AUX_CREATE for the same arguments indicates successful
     * RTT granule state transition */
    cmd_ret = val_host_rmi_rtt_aux_create(c_args.rd_valid, undelegated_rtt, c_args.ipa_valid,
                                                 c_args.level_valid, c_args.index_valid);
    if (cmd_ret.x0 != 0)
    {
        LOG(ERROR, " RTT_AUX_CREATE failed. %x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    /* Unfold folded RTTs to meet postamble requirements */
    if (val_host_create_aux_mapping(c_args.rd_valid, c_args.ipa_valid, RTT_INDEX_1))
    {
        LOG(ERROR, " RTT_AUX_CREATE failed. %x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    if (val_host_create_aux_mapping(c_args.rd_valid, IPA_FOLDED_RTT, RTT_INDEX_1))
    {
        LOG(ERROR, " RTT_AUX_CREATE failed. %x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
