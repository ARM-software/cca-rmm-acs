/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_data_destroy_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define MAX_GRANULES 256

#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512 * L3_SIZE)
#define L1_SIZE (512 * L2_SIZE)

#define IPA_ADDR_DATA 3 * PAGE_SIZE
#define IPA_ADDR_ASSIGNED PAGE_SIZE

#define MAP_LEVEL 3

#define NUM_REALMS 1
#define VALID_REALM 0

/*      Valid Realm memory layout
 *
 * ipa: 0x0 (1st protected L3 Entry)
 *  ------------------------------------
 * |  HIPAS = UNASSIGNED, RIPAS = RAM   |
 *  ------------------------------------
 * |   HIPAS = ASSIGNED, RIPAS = RAM    |
 *  ------------------------------------
 * |   HIPAS = DESTROYED                |
  *  ------------------------------------
 * |  HIPAS = ASSIGNED, RIPAS = RAM     |
 *  ------------------------------------
 * |  HIPAS = ASSIGNED, RIPAS = EMPTY   |
 *  ------------------------------------
 * ipa: 0x5000
 *
 *                  (...)
 *
 *
 * ipa: 0x1XXX000 (1st unprotected L3 Entry)
 *  ------------------------------------
 * |         HIPAS = ASSIGNED_NS        |
 *  ------------------------------------
 * |        HIPAS = UNASSIGNED          |
 *  ------------------------------------
 * ipa: 0x1XX2000
 *
 */

static val_host_realm_ts realm_test[NUM_REALMS];

static struct argument_store {
    uint64_t rd_valid;
    uint64_t ipa_valid;
} c_args;

struct arguments {
    uint64_t rd;
    uint64_t ipa;
};

struct cmd_output {
    uint64_t top;
    uint64_t data;
} c_exp_output;

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
        LOG(ERROR, "\tRec Create Failed\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm.rec[0];
}

static uint64_t ipa_valid_prep_sequence(void)
{
    /* RTTE[ipa].state = ASSIGNED
       RTTE[ipa].ripas = RAM/EMPTY */

    if (create_mapping(IPA_ADDR_ASSIGNED, true, c_args.rd_valid))
    {
        LOG(ERROR, "\tCouldn't create the assigned protected mapping\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    uint64_t data = g_delegated_prep_sequence();
    if (data == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    uint64_t src = g_undelegated_prep_sequence();
    if (src == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    uint64_t flags = RMI_NO_MEASURE_CONTENT;

    if (val_host_rmi_data_create(c_args.rd_valid, data, IPA_ADDR_ASSIGNED, src, flags))
    {
        LOG(ERROR, "\tCouldn't complete the assigned protected mapping\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    /* save the PA before returning*/
    c_exp_output.data = data;
    return IPA_ADDR_ASSIGNED;
}

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

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.rd_valid = rd_valid_prep_sequence();
    if (c_args.rd_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.ipa_valid = ipa_valid_prep_sequence();
    if (c_args.ipa_valid == VAL_TEST_PREP_SEQ_FAILED)
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
            args->ipa = c_args.ipa_valid;
            break;

        case RD_DEV_MEM_MMIO:
            args->rd = g_dev_mem_prep_sequence();
            args->ipa = c_args.ipa_valid;
            break;

        case RD_OUTSIDE_OF_PERMITTED_PA:
            args->rd = g_outside_of_permitted_pa_prep_sequence();
            args->ipa = c_args.ipa_valid;
            break;

        case RD_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            break;

        case RD_STATE_DELEGATED:
            args->rd = g_delegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            break;

        case RD_STATE_REC:
            args->rd = g_rec_ready_prep_sequence(c_args.rd_valid);
            args->ipa = c_args.ipa_valid;
            break;

        case RD_STATE_RTT:
            args->rd = realm_test[VALID_REALM].rtt_l0_addr;
            args->ipa = c_args.ipa_valid;
            break;

        case RD_STATE_DATA:
            args->rd = g_data_prep_sequence(c_args.rd_valid, IPA_ADDR_DATA);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            break;

        case IPA_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->ipa = g_unaligned_prep_sequence(c_args.ipa_valid);
            break;

        case IPA_UNPROTECTED:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_unprotected_unassigned_prep_sequence(c_args.rd_valid);
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case IPA_OUTSIDE_OF_PERMITTED_IPA:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_outside_of_permitted_ipa_prep_sequence();
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case IPA_NOT_MAPPED:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_protected_unmapped_prep_sequence();
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            c_exp_output.top = L1_SIZE;
            break;

        case RTTE_STATE_UNASSIGNED:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_protected_unassigned_ram_prep_sequence(c_args.rd_valid);
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            c_exp_output.top = PAGE_SIZE;
            break;

        case IPA_UNPROTECTED_NOT_MAPPED:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_unprotected_unmapped_prep_sequence();
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case IPA_UNPROTECTED_RTTE_UNASSIGNED:
            args->rd = c_args.rd_valid;
            args->ipa = ipa_unprotected_unassigned_prep_sequence(c_args.rd_valid);
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        default:
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }
    return VAL_SUCCESS;
}

void cmd_data_destroy_host(void)
{
    uint64_t ret = 0, i, data;
    struct arguments args;
    val_host_rtt_entry_ts rtte;
    val_host_data_destroy_ts output_val;

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

        ret = val_host_rmi_data_destroy(args.rd, args.ipa, &output_val);
        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }

        /* Upon RMI_ERROR_RTT check for top == walk_top */
        else if (ret == RMI_ERROR_RTT && output_val.top != c_exp_output.top) {
            LOG(ERROR, " \tUnexpected command output received, top value: 0x%x"
                                                            , output_val.top, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);

    ret = val_host_rmi_data_destroy(c_args.rd_valid, c_args.ipa_valid, &output_val);
    if (ret != 0)
    {
        LOG(ERROR, "\n\tDATA_DESTROY command failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    if (output_val.data != c_exp_output.data || output_val.top != IPA_ADDR_DATA) {
        LOG(ERROR, "\n\tUnexpected output values, data: 0x%x, top: 0x%x\n",
                                                     output_val.data, output_val.top);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    /* Check for RIPAS and HIPAS from RIPAS = RAM */
    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                      3, &rtte);
    if (ret) {
        LOG(ERROR, "\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    } else {
        if (rtte.state != RMI_UNASSIGNED || rtte.ripas != RMI_DESTROYED)
        {
            LOG(TEST, "\tUnexpected HIPAS and RIPAS values received. HIPAS: %d, RIPAS: %d ",
                                                                     rtte.state, rtte.ripas);
            LOG(ERROR, "\tExpected. HIPAS: %d, RIPAS: %d ", RMI_UNASSIGNED, RMI_DESTROYED);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
            goto exit;
        }
    }

    /* Check for RIPAS transition from ASSIGNED,DESTROYED */
    data = g_delegated_prep_sequence();
    if (data == VAL_TEST_PREP_SEQ_FAILED) {
        LOG(ERROR, "\t Granule delegation failed", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

    ret = val_host_rmi_data_create_unknown(c_args.rd_valid, data, c_args.ipa_valid);
    if (ret) {
        LOG(ERROR, "\t DATA_CREATE_UNKNOWN failed with ret value %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto exit;
    }

    ret = val_host_rmi_data_destroy(c_args.rd_valid, c_args.ipa_valid, &output_val);
    if (ret != 0)
    {
        LOG(ERROR, "\n\tDATA_DESTROY command failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto exit;
    }

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                      3, &rtte);
    if (ret) {
        LOG(ERROR, "\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto exit;
    } else {
        if (rtte.state != RMI_UNASSIGNED || rtte.ripas != RMI_DESTROYED)
        {
            LOG(TEST, "\tUnexpected HIPAS and RIPAS values received. HIPAS: %d, RIPAS: %d ",
                                                                     rtte.state, rtte.ripas);
            LOG(ERROR, "\tExpected. HIPAS: %d, RIPAS: %d ", RMI_UNASSIGNED, RMI_EMPTY);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
            goto exit;
        }
    }

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, (4 * PAGE_SIZE),
                                      3, &rtte);
    if (ret) {
        LOG(ERROR, "\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(16)));
        goto exit;
    }

    /* Check for RIPAS and HIPAS from RIPAS = EMPTY */
    uint64_t ipa = ipa_protected_assigned_empty_prep_sequence(c_args.rd_valid);
    if (ipa == VAL_TEST_PREP_SEQ_FAILED) {
        LOG(ERROR, "\t Protected IPA preparation sequence failed", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(14)));
        goto exit;
    }

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, ipa,
                                      3, &rtte);
    if (ret) {
        LOG(ERROR, "\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(16)));
        goto exit;
    }

    ret = val_host_rmi_data_destroy(c_args.rd_valid, ipa, &output_val);
    if (ret != 0)
    {
        LOG(ERROR, "\n\tDATA_DESTROY command failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(15)));
        goto exit;
    }

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, ipa,
                                      3, &rtte);
    if (ret) {
        LOG(ERROR, "\tREAD_ENTRY failed with ret value: %d\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(16)));
        goto exit;
    } else {
        if (rtte.state != RMI_UNASSIGNED || rtte.ripas != RMI_EMPTY)
        {
            LOG(TEST, "\tUnexpected HIPAS and RIPAS values received. HIPAS: %d, RIPAS: %d ",
                                                                     rtte.state, rtte.ripas);
            LOG(ERROR, "\tExpected. HIPAS: %d, RIPAS: %d ", RMI_UNASSIGNED, RMI_EMPTY);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(17)));
            goto exit;
        }
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
