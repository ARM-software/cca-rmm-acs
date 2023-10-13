/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_data_create_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define MAX_GRANULES 256

#define L3_SIZE PAGE_SIZE
#define L2_SIZE (512 * L3_SIZE)
#define L1_SIZE (512 * L2_SIZE)

/*      Valid Realm memory layout
 *
 * ipa: 0x0 (1st protected L3 Entry)
 *  ------------------------------------
 * |  HIPAS = UNASSIGNED, RIPAS = RAM   |
 *  ------------------------------------
 * |    HIPAS = ASSIGNED, RIPAS = RAM   |
 *  ------------------------------------
 * |         HIPAS = DESTROYED          |
 *  ------------------------------------
 * |  HIPAS = UNASSIGNED, RIPAS = EMPTY |
 *  ------------------------------------
 * |  HIPAS = UNASSIGNED, RIPAS = EMPTY |
 *  ------------------------------------
 * |    HIPAS = ASSIGNED, RIPAS = RAM   |
 *  ------------------------------------
 * |    HIPAS = ASSIGNED, RIPAS = RAM   |
 *  ------------------------------------
 * ipa: 0x7000
 *
 *                  (...)
 *
 *
 * ipa: 0x1XXX000 (1st unprotected L3 Entry)
 *  ------------------------------------
 * |         HIPAS = ASSIGNED_NS        |
 *  ------------------------------------
 * |        HIPAS = UNASSIGNED_NS       |
 *  ------------------------------------
 * ipa: 0x1XX2000
 *
 */

#define IPA_ADDR_UNASSIGNED_RAM 0x0
#define IPA_ADDR_DATA1  (5 * PAGE_SIZE)
#define IPA_ADDR_DATA2  (6 * PAGE_SIZE)

#define MAP_LEVEL 3

#define NUM_REALMS 4
#define VALID_REALM 0
#define ACTIVE_REALM 1
#define NULL_REALM 2
#define SYSTEM_OFF_REALM 3

static val_host_realm_ts realm_test[NUM_REALMS];

static struct argument_store {
    uint64_t rd_valid;
    uint64_t data_valid;
    uint64_t ipa_valid;
    uint64_t src_valid;
    uint64_t flags_valid;
} c_args;

static struct invalid_argument_store {
    uint64_t rec_gran;
} c_args_invalid;

struct arguments {
    uint64_t rd;
    uint64_t data;
    uint64_t ipa;
    uint64_t src;
    uint64_t flags;
};

static uint64_t data_valid_prep_sequence(void)
{
    return g_delegated_prep_sequence();
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

static uint64_t ipa_valid_prep_sequence(void)
{
    if (create_mapping(IPA_ADDR_UNASSIGNED_RAM, true, c_args.rd_valid))
    {
        LOG(ERROR, "\tCouldn't create the protected mapping\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    return IPA_ADDR_UNASSIGNED_RAM;
}

static uint64_t src_valid_prep_sequence(void)
{
    return g_undelegated_prep_sequence();
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


static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.rd_valid = rd_valid_prep_sequence();
    if (c_args.rd_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;


    c_args.data_valid = data_valid_prep_sequence();
    if (c_args.data_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.ipa_valid = ipa_valid_prep_sequence();
    if (c_args.ipa_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.src_valid = src_valid_prep_sequence();
    if (c_args.src_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.flags_valid = RMI_MEASURE_CONTENT;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case SRC_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->data = c_args.data_valid;
            args->ipa = c_args.ipa_valid;
            args->src = g_unaligned_prep_sequence(c_args.src_valid);
            args->rd = c_args.rd_valid;
            args->flags = c_args.flags_valid;
            break;

        case SRC_DEV_MEM_MMIO:
            args->rd = c_args.rd_valid;
            args->data = c_args.data_valid;
            args->ipa = c_args.ipa_valid;
            args->src = g_dev_mem_prep_sequence();
            args->flags = c_args.flags_valid;
            break;

        case SRC_OUTSIDE_OF_PERMITTED_PA:
            args->rd = c_args.rd_valid;
            args->data = c_args.data_valid;
            args->ipa = c_args.ipa_valid;
            args->src = g_outside_of_permitted_pa_prep_sequence();
            args->flags = c_args.flags_valid;
            break;

        case SRC_PAS_REALM:
            args->rd = c_args.rd_valid;
            args->data = c_args.data_valid;
            args->ipa = c_args.ipa_valid;
            args->src = g_delegated_prep_sequence();
            if (args->src == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->flags = c_args.flags_valid;
            break;

        case SRC_PAS_SECURE:
            args->rd = c_args.rd_valid;
            args->data = c_args.data_valid;
            args->ipa = c_args.ipa_valid;
            args->src = g_secure_prep_sequence();
            args->flags = c_args.flags_valid;
            break;

        case DATA_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->data = g_unaligned_prep_sequence(c_args.data_valid);
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case DATA_DEV_MEM_MMIO:
            args->rd = c_args.rd_valid;
            args->data = g_dev_mem_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case DATA_OUTSIDE_OF_PERMITTED_PA:
            args->rd = c_args.rd_valid;
            args->data = g_outside_of_permitted_pa_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case DATA_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->data = g_undelegated_prep_sequence();
            if (args->data == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case DATA_STATE_REC:
            args->rd = c_args.rd_valid;
            args->data = g_rec_ready_prep_sequence(c_args.rd_valid);
            if (args->data == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            c_args_invalid.rec_gran = args->data;
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case DATA_STATE_RD:
            args->rd = c_args.rd_valid;
            args->data = c_args.rd_valid;
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case DATA_STATE_RTT:
            args->rd = c_args.rd_valid;
            args->data = realm_test[VALID_REALM].rtt_l0_addr;
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case DATA_STATE_DATA:
            args->rd = c_args.rd_valid;
            args->data = g_data_prep_sequence(c_args.rd_valid, IPA_ADDR_DATA1);
            if (args->data == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case DATA_PAS_SECURE:
            args->rd = c_args.rd_valid;
            args->data = g_secure_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            args->data = c_args.data_valid;
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case RD_DEV_MEM_MMIO:
            args->rd = g_dev_mem_prep_sequence();
            args->data = c_args.data_valid;
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case RD_OUTSIDE_OF_PERMITTED_PA:
            args->rd = g_outside_of_permitted_pa_prep_sequence();
            args->data = c_args.data_valid;
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case RD_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->data = c_args.data_valid;
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case RD_STATE_DELEGATED:
            args->rd = g_delegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->data = c_args.data_valid;
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case RD_STATE_REC:
            args->rd = c_args_invalid.rec_gran;
            args->data = c_args.data_valid;
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case RD_STATE_RTT:
            args->rd = realm_test[VALID_REALM].rtt_l0_addr;
            args->data = c_args.data_valid;
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case RD_STATE_DATA:
            args->rd = g_data_prep_sequence(c_args.rd_valid, IPA_ADDR_DATA2);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->data = c_args.data_valid;
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case IPA_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->data = c_args.data_valid;
            args->ipa = g_unaligned_prep_sequence(c_args.ipa_valid);
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case IPA_UNPROTECTED:
            args->rd = c_args.rd_valid;
            args->data = c_args.data_valid;
            args->ipa = ipa_unprotected_unassigned_prep_sequence(c_args.rd_valid);
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case IPA_OUTSIDE_OF_PERMITTED_IPA:
            args->rd = c_args.rd_valid;
            args->data = c_args.data_valid;
            args->ipa = ipa_outside_of_permitted_ipa_prep_sequence();
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case REALM_ACTIVE:
            args->rd = g_rd_active_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->data = c_args.data_valid;
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case REALM_SYSTEM_OFF:
            args->rd = g_rd_system_off_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->data = c_args.data_valid;
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case REALM_NULL:
            args->rd = g_rd_null_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->data = c_args.data_valid;
            args->ipa = c_args.ipa_valid;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case IPA_NOT_MAPPED:
            args->rd = c_args.rd_valid;
            args->data = c_args.data_valid;
            args->ipa = ipa_protected_unmapped_prep_sequence();
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case RTTE_STATE_ASSIGNED:
            args->rd = c_args.rd_valid;
            args->data = c_args.data_valid;
            args->ipa = ipa_protected_assigned_ram_prep_sequence(c_args.rd_valid);
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case RIPAS_DESTROYED:
            args->rd = c_args.rd_valid;
            args->data = c_args.data_valid;
            args->ipa = ipa_protected_destroyed_prep_sequence(c_args.rd_valid);
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case RIPAS_EMPTY:
            args->rd = c_args.rd_valid;
            args->data = c_args.data_valid;
            args->ipa = ipa_protected_unassigned_empty_prep_sequence(c_args.rd_valid);
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case IPA_UNPROTECTED_NOT_MAPPED:
            args->rd = c_args.rd_valid;
            args->data = c_args.data_valid;
            args->ipa = ipa_unprotected_unmapped_prep_sequence();
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        case IPA_UNPROTECTED_RTTE_ASSIGNED:
            args->rd = c_args.rd_valid;
            args->data = c_args.data_valid;
            args->ipa = ipa_unprotected_assinged_prep_sequence(c_args.rd_valid);
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->src = c_args.src_valid;
            args->flags = c_args.flags_valid;
            break;

        default:
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }
    return VAL_SUCCESS;
}

void cmd_data_create_host(void)
{
    uint64_t ret = 0, i;
    struct arguments args;
    val_host_rtt_entry_ts rtte;

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

        ret = val_host_rmi_data_create(args.rd, args.data, args.ipa, args.src, args.flags);

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);

    ret = val_host_rmi_data_create(c_args.rd_valid, c_args.data_valid,
                              c_args.ipa_valid, c_args.src_valid, c_args.flags_valid);

    if (ret != 0)
    {
        LOG(ERROR, "\n\tData Create command failed. %x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    /* Check that rtte.addr and rtte.state have changed */
    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                      MAP_LEVEL, &rtte);
    if (ret) {
        LOG(ERROR, "\tREAD_ENTRY failed!\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    } else {
        if (rtte.state != RMI_ASSIGNED || OA(rtte.desc) != c_args.data_valid
                                                   || rtte.ripas != RMI_RAM)
        {
            LOG(ERROR, "\tState was: %d & OA was: %x\n", rtte.state, OA(rtte.desc));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
            goto exit;
        }
    }
    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
