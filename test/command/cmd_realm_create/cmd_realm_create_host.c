/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_realm_create_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define MAX_VMID 65535 // 16-bit VMID
#define MAX_GRANULES 256

#define NUM_REALMS 2
#define REALM_VALID 0
#define REALM_NEW 1

#define IPA_ADDR_DATA 0

static val_host_realm_ts realm[NUM_REALMS];

static struct argument_store {
    uint64_t rd_valid;
    uint64_t params_valid;
    uint64_t dev_mem;
    uint64_t delegated;
} c_args;

struct arguments {
    uint64_t rd;
    uint64_t params_ptr;
};

typedef enum {
    PARAMS_HASH_INVALID = 0x0,
    PARAMS_HASH_UNSUPPORTED,
    PARAMS_RTT_UNALLIGNED,
    PARAMS_INVALID_FEAT_REG,
    PARAMS_INVALID_RTT_START,
    PARAMS_RTT_UNDELEGATED,
    PARAMS_VMID_INVALID,
    PARAMS_VMID_USED
} prep_seq_type;

static uint64_t rd_valid_prep_sequence(void)
{
    uint64_t rd = g_delegated_prep_sequence();
    if (rd == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    /* Track for reuse/destruction */
    realm[REALM_VALID].rd = rd;

    return rd;
}

static uint64_t g_params_prep_sequence(prep_seq_type type)
{
    /*  Allocate a granule for RealmParams */
    val_host_realm_params_ts *params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    /* Allocate and delegate a granule for the L0 RTT */
    uint64_t rtt_base = realm[REALM_VALID].rtt_l0_addr;

    uint64_t featreg0, ret;
    uint8_t s2sz, supported_hashalgo, vmidbits;

    params->hash_algo = RMI_SHA256;
    params->realm_feat_0 = IPA_WIDTH;
    params->num_s2_sl_rtts = 1;
    params->s2_starting_level = 0;
    params->vmid = 0;
    params->rtt_addr = rtt_base;

    switch (type)
    {
        case PARAMS_HASH_INVALID:
            params->hash_algo = 2;
            break;

        case PARAMS_HASH_UNSUPPORTED:

            ret = val_host_rmi_features(0, &featreg0);
            if (ret) {
                LOG(TEST, "\tFeature Read failed, ret=%x\n", ret, 0);
                return VAL_TEST_PREP_SEQ_FAILED;
            }
            supported_hashalgo = VAL_EXTRACT_BITS(featreg0, 28, 29);

            if (supported_hashalgo == 0x1)
            params->hash_algo = 1;
            else if (supported_hashalgo == 0x2)
            params->hash_algo = 0;
            else {
                LOG(TEST, "\n\tNo Invalid combitation", 0, 0);
                return VAL_SKIP_CHECK;
            }
            break;

        case PARAMS_RTT_UNALLIGNED:
            params->rtt_addr = g_unaligned_prep_sequence(rtt_base);
            break;

        case PARAMS_INVALID_FEAT_REG:
            params->realm_feat_0 = 38;
            break;

        case PARAMS_INVALID_RTT_START:
            ret = val_host_rmi_features(0, &featreg0);
            if (ret)
            LOG(TEST, "\tFeature Read failed, ret=%x\n", ret, 0);

            s2sz = VAL_EXTRACT_BITS(featreg0, 0, 7);

            if (s2sz > 48 && s2sz <= 52) {
                params->realm_feat_0 = 1U << 8 | s2sz;
                params->s2_starting_level = 1;
            }
            else if (s2sz > 39 && s2sz <= 48) {
                params->realm_feat_0 = s2sz;
                params->s2_starting_level = 2;
            }
            else if (s2sz > 30 && s2sz <= 39) {
                params->realm_feat_0 = s2sz;
                params->s2_starting_level = 3;
            }
            else {
                params->realm_feat_0 = s2sz;
                params->s2_starting_level = 2;
            }
            break;

        case PARAMS_RTT_UNDELEGATED:
            params->rtt_addr = g_undelegated_prep_sequence();
            if (params->rtt_addr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_TEST_PREP_SEQ_FAILED;
            break;

        case PARAMS_VMID_INVALID:
            vmidbits = VAL_EXTRACT_BITS(val_id_aa64mmfr1_el1_read(), 4, 7);

            if (vmidbits == 0x1)
                params->vmid = 0xFF00;
            else {
                LOG(TEST, "\n\tCouldn't create VMID Invalid sequence", 0, 0);
                return VAL_SKIP_CHECK;
            }
            break;

        case PARAMS_VMID_USED:
            params->vmid = 1;
            break;

        default:
            LOG(ERROR, "\n\t INVALID PREP_SEQUENCE \n", 0, 0);
            return VAL_TEST_PREP_SEQ_FAILED;
    }
    return (uint64_t)params;
}

static uint64_t params_valid_prep_sequence(void)
{
    // Allocate a granule for RealmParams
    val_host_realm_params_ts *params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    // Allocate and delegate a granule for the L0 RTT
    uint64_t rtt0 = g_delegated_prep_sequence();

    realm[REALM_VALID].rtt_l0_addr = rtt0;

    params->hash_algo = RMI_SHA256;
    // alp12: params->hash_algo = RMI_SHA256
    params->realm_feat_0 = IPA_WIDTH;
    params->num_s2_sl_rtts = 1;
    params->s2_starting_level = 0;
    params->vmid = REALM_VALID;
    params->rtt_addr = rtt0;

    return (uint64_t)params;
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
    realm[vmid].rd = realm_init.rd;
    realm[vmid].rtt_l0_addr = realm_init.rtt_l0_addr;
    return realm_init.rd;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.rd_valid = rd_valid_prep_sequence();
    if (c_args.rd_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.params_valid = params_valid_prep_sequence();
    if (c_args.params_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case PARAMS_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_unaligned_prep_sequence(c_args.params_valid);
            break;

        case PARAMS_DEV_MEM:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_dev_mem_prep_sequence();
            break;

        case PARAMS_OUTSIDE_OF_PERMITTED_PA:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_outside_of_permitted_pa_prep_sequence();
            break;

        case PARAMS_PAS_REALM:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_delegated_prep_sequence();
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case PARAMS_PAS_SECURE:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_secure_prep_sequence();
            break;

        case HASH_ALGO_INVALID:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_HASH_INVALID);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case HASH_ALGO_UNSUPPORTED:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_HASH_UNSUPPORTED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            else if (args->params_ptr == VAL_SKIP_CHECK)
                return VAL_SKIP_CHECK;
            break;

        case RTT_BASE_RD_ALIASED:
            args->rd = realm[REALM_VALID].rtt_l0_addr;
            args->params_ptr = c_args.params_valid;
            break;

        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            args->params_ptr = c_args.params_valid;
            break;

        case RD_DEV_MEM:
            args->rd = g_dev_mem_prep_sequence();
            args->params_ptr = c_args.params_valid;
            break;

        case RD_OUTSIDE_OF_PERMITTED_PA:
            args->rd = g_outside_of_permitted_pa_prep_sequence();
            args->params_ptr = c_args.params_valid;
            break;

        case RD_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            args->params_ptr = c_args.params_valid;
            break;

        case RD_STATE_RD:
            args->rd = g_rd_new_prep_sequence(REALM_NEW);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->params_ptr = c_args.params_valid;
            break;

        case RD_STATE_REC:
            args->rd = g_rec_ready_prep_sequence(realm[REALM_NEW].rd);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_TEST_PREP_SEQ_FAILED;
            args->params_ptr = c_args.params_valid;
            break;

        case RD_STATE_RTT:
            args->rd = realm[REALM_NEW].rtt_l0_addr;
            args->params_ptr = c_args.params_valid;
            break;

        case RD_STATE_DATA:
            args->rd = g_data_prep_sequence(realm[REALM_NEW].rd, IPA_ADDR_DATA);
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_TEST_PREP_SEQ_FAILED;
            args->params_ptr = c_args.params_valid;
            break;

        case RTT_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_RTT_UNALLIGNED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case FEAT_REG_INVALID:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_INVALID_FEAT_REG);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case RTT_START_INVALID:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_INVALID_RTT_START);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case RTT_BASE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_RTT_UNDELEGATED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case VMID_INVALID:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_VMID_INVALID);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            else if (args->params_ptr == VAL_SKIP_CHECK)
                return VAL_SKIP_CHECK;
            break;

        case VMID_USED:
            args->rd = c_args.rd_valid;
            args->params_ptr = g_params_prep_sequence(PARAMS_VMID_USED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        default:
            /* set status to failure */
            LOG(ERROR, "\n\tUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}


void cmd_realm_create_host(void)
{
    uint64_t ret;
    struct arguments args = {0, 0};
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

        ret = intent_to_seq(&test_data[i], &args);
        if (ret == VAL_SKIP_CHECK)
        {
            LOG(TEST, "\n\tSkipping Test case\n", 0, 0);
            continue;
        }
        else if (ret == VAL_ERROR) {
            LOG(ERROR, "\n\t Intent to sequence failed \n", 0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto fail;
        }

        ret = val_host_rmi_realm_create(args.rd, args.params_ptr);

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index))
        {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                            ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto fail;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);
    ret = val_host_rmi_realm_create(c_args.rd_valid, c_args.params_valid);
    /* Valid call should give success if footprints have not changed */
    if (ret != 0) {
        LOG(ERROR, "\n\tPositive Observability Check Failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto fail;
    }

    LOG(TEST, "\n\tNegative Observability Check\n", 0, 0);
    ret = val_host_rmi_realm_create(c_args.rd_valid, c_args.params_valid);
    /* Now the command should fail with the same parameters (footprint has changed) */
    if (ret == 0) {
        LOG(ERROR, "\n\tNegative Observability Check Failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto fail;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));
    return;

fail:
    return;
}
