/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_rec_create_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define MAX_VMID 65535 // 16-bit VMID
#define MAX_GRANULES 256

#define MPIDR_VALID 0

#define NUM_REALMS 4
#define REALM_VALID 0
#define ACTIVE_REALM 1
#define NEW_REALM 2
#define SYSTEM_OFF_REALM 3

#define IPA_ADDR_DATA 0

static val_host_realm_ts realm_test[NUM_REALMS];

static struct argument_store {
    uint64_t rd_valid;
    uint64_t rec_valid;
    uint64_t params_valid;
} c_args;

static struct invalid_arguments {
    uint64_t rec_gran;
    uint64_t data_gran;
} c_args_invalid;

struct arguments {
    uint64_t rd;
    uint64_t rec;
    uint64_t params_ptr;
};

typedef enum {
    PARAMS_VALID = 0x0,
    SKIPPED_MPIDR,
    AUX_GRAN_UNALINED,
    INVALID_NUM_AUX,
    PARAMS_AUX_UNDELEGATED,
    PARAMS_AUX_RD,
    PARAMS_AUX_REC,
    PARAMS_AUX_DATA,
    PARAMS_AUX_RTT,
    PARAMS_AUX_ALIASED,
} prep_seq_type;

static uint64_t rec_valid_prep_sequence(void)
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
    return g_rd_new_prep_sequence(REALM_VALID);
}

static uint64_t params_prep_sequence(prep_seq_type type)
{
    val_host_rec_params_ts *params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    /* Zero out the gprs */
    uint64_t i;
    for (i = 0; i < (sizeof(params->gprs) / sizeof(params->gprs[0])); i++)
        params->gprs[i] = 0x0;

    /* Fetch the rd of the Realm */
    uint64_t rd = realm_test[REALM_VALID].rd;

    /* Get aux granules count */
    uint64_t aux_count;
    if (val_host_rmi_rec_aux_count(rd, &aux_count)) {
        LOG(ERROR, "\tCouldn't obtain the required amount of AUX granules\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Set these fields to some value */
    params->pc = 0;
    params->flags = RMI_RUNNABLE;
    params->mpidr = MPIDR_VALID;
    params->num_aux = aux_count;

    /* Create all aux granules */
    for (i = 0; i < aux_count; i++) {
        uint64_t aux_rec = g_delegated_prep_sequence();
        if (aux_rec == VAL_TEST_PREP_SEQ_FAILED)
            return VAL_TEST_PREP_SEQ_FAILED;

        params->aux[i] = aux_rec;
    }

    switch (type)
    {
        case PARAMS_VALID:
            break;

        case SKIPPED_MPIDR:
            params->mpidr = 0xF;
            break;

        case AUX_GRAN_UNALINED:
             for (i = 0; i < aux_count; i++) {
                uint64_t aux_rec = g_delegated_prep_sequence();
                if (aux_rec == VAL_TEST_PREP_SEQ_FAILED)
                    return VAL_TEST_PREP_SEQ_FAILED;
                params->aux[i] = g_unaligned_prep_sequence(aux_rec);
            }
            break;

        case INVALID_NUM_AUX:
            params->num_aux = aux_count + 1;
            break;

        case PARAMS_AUX_UNDELEGATED:
            for (i = 0; i < aux_count; i++)
                params->aux[i] = g_undelegated_prep_sequence();
            break;

        case PARAMS_AUX_RD:
            for (i = 0; i < aux_count; i++)
                params->aux[i] = c_args.rd_valid;
            break;

        case PARAMS_AUX_REC:
            for (i = 0; i < aux_count; i++)
                params->aux[i] = c_args_invalid.rec_gran;
            break;

        case PARAMS_AUX_DATA:
            for (i = 0; i < aux_count; i++)
                params->aux[i] = c_args_invalid.data_gran;
            break;

        case PARAMS_AUX_RTT:
            for (i = 0; i < aux_count; i++)
                params->aux[i] = realm_test[REALM_VALID].rtt_l0_addr;
            break;

        case PARAMS_AUX_ALIASED:
            params->aux[1] = params->aux[0];
            break;

        default:
            return VAL_TEST_PREP_SEQ_FAILED;
    }

    return (uint64_t)params;
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


static uint64_t g_rec_ready_prep_sequence(void)
{
    val_host_rec_params_ts rec_params;

    if (g_rd_new_prep_sequence(NEW_REALM) == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    /* Track for reuse/destruction */

    realm_test[NEW_REALM].rec_count = 1;
    rec_params.pc = 0;
    rec_params.flags = RMI_RUNNABLE;
    rec_params.mpidr = 0;

    if (val_host_rec_create_common(&realm_test[NEW_REALM], &rec_params)) {
        LOG(ERROR, "\n\t REC Create Failed \n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm_test[NEW_REALM].rec[0];
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.rec_valid = rec_valid_prep_sequence();
    if (c_args.rec_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.rd_valid = rd_valid_prep_sequence();
    if (c_args.rd_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.params_valid = params_prep_sequence(PARAMS_VALID);
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
            args->rec = c_args.rec_valid;
            args->params_ptr = g_unaligned_prep_sequence(c_args.params_valid);
            break;

        case PARAMS_DEV_MEM:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rec_valid;
            args->params_ptr = g_dev_mem_prep_sequence();
            break;

        case PARAMS_OUTSIDE_OF_PERMITTED_PA:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rec_valid;
            args->params_ptr = g_outside_of_permitted_pa_prep_sequence();
            break;

        case PARAMS_PAS_REALM:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rec_valid;
            args->params_ptr = g_delegated_prep_sequence();
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case PARAMS_PAS_SECURE:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rec_valid;
            args->params_ptr = g_secure_prep_sequence();
            break;

        case REC_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rec = g_unaligned_prep_sequence(c_args.rec_valid);
            args->params_ptr = c_args.params_valid;
            break;

        case REC_DEV_MEM:
            args->rd = c_args.rd_valid;
            args->rec = g_dev_mem_prep_sequence();
            args->params_ptr = c_args.params_valid;
            break;

        case REC_OUTSIDE_OF_PERMITTED_PA:
            args->rd = c_args.rd_valid;
            args->rec = g_outside_of_permitted_pa_prep_sequence();
            args->params_ptr = c_args.params_valid;
            break;

        case REC_GRAN_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->rec = g_undelegated_prep_sequence();
            if (args->rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->params_ptr =  c_args.params_valid;
            break;

        case REC_GRAN_STATE_RD:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rd_valid;
            args->params_ptr = c_args.params_valid;
            break;

        case REC_GRAN_STATE_REC:
            args->rd = realm_test[NEW_REALM].rd;
            args->rec = g_rec_ready_prep_sequence();
            if (args->rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            c_args_invalid.rec_gran = args->rec;
            args->params_ptr = c_args.params_valid;
            break;

        case REC_GRAN_STATE_RTT:
            args->rd = c_args.rd_valid;
            args->rec = realm_test[REALM_VALID].rtt_l0_addr;
            args->params_ptr = c_args.params_valid;
            break;

        case REC_GRAN_STATE_DATA:
            args->rd = c_args.rd_valid;
            args->rec = g_data_prep_sequence(c_args.rd_valid, IPA_ADDR_DATA);
            if (args->rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            c_args_invalid.data_gran = args->rec;
            args->params_ptr = c_args.params_valid;
            break;

        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            args->rec = c_args.rec_valid;
            args->params_ptr = c_args.params_valid;
            break;

        case RD_DEV_MEM:
            args->rd = g_dev_mem_prep_sequence();
            args->rec = c_args.rec_valid;
            args->params_ptr = c_args.params_valid;
            break;

        case RD_OUTSIDE_OF_PERMITTED_PA:
            args->rd = g_outside_of_permitted_pa_prep_sequence();
            args->rec = c_args.rec_valid;
            args->params_ptr = c_args.params_valid;
            break;

        case RD_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->rec = c_args.rec_valid;
            args->params_ptr = c_args.params_valid;
            break;

        case RD_STATE_DELEGATED:
            args->rd = g_delegated_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->rec = c_args.rec_valid;
            args->params_ptr = c_args.params_valid;
            break;

        case RD_STATE_REC:
            args->rd = c_args_invalid.rec_gran;
            args->rec = c_args.rec_valid;
            args->params_ptr = c_args.params_valid;
            break;

        case RD_STATE_RTT:
            args->rd = realm_test[REALM_VALID].rtt_l0_addr;
            args->rec = c_args.rec_valid;
            args->params_ptr = c_args.params_valid;
            break;

        case RD_STATE_DATA:
            args->rd = c_args_invalid.data_gran;
            args->rec = c_args.rec_valid;
            args->params_ptr = c_args.params_valid;
            break;

        case REALM_ACTIVE:
            args->rd = g_rd_active_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->rec = c_args.rec_valid;
            args->params_ptr = c_args.params_valid;
            break;

        case REALM_SYSTEM_OFF:
            args->rd = g_rd_system_off_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->rec = c_args.rec_valid;
            args->params_ptr = c_args.params_valid;
            break;

        case MPIDR_SKIPPED:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rec_valid;
            args->params_ptr = params_prep_sequence(SKIPPED_MPIDR);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case NUM_AUX_INVALID:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rec_valid;
            args->params_ptr = params_prep_sequence(INVALID_NUM_AUX);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case AUX_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rec_valid;
            args->params_ptr = params_prep_sequence(AUX_GRAN_UNALINED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case AUX_ALIASED:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rec_valid;
            args->params_ptr = params_prep_sequence(PARAMS_AUX_ALIASED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case AUX_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rec_valid;
            args->params_ptr = params_prep_sequence(PARAMS_AUX_UNDELEGATED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case AUX_RD:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rec_valid;
            args->params_ptr = params_prep_sequence(PARAMS_AUX_RD);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case AUX_REC:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rec_valid;
            args->params_ptr = params_prep_sequence(PARAMS_AUX_REC);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case AUX_RTT:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rec_valid;
            args->params_ptr = params_prep_sequence(PARAMS_AUX_RTT);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case AUX_DATA:
            args->rd = c_args.rd_valid;
            args->rec = c_args.rec_valid;
            args->params_ptr = params_prep_sequence(PARAMS_AUX_DATA);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        default:
            /* set status to failure */
            LOG(ERROR, "\t\nUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}


void cmd_rec_create_host(void)
{
    uint64_t ret = 0;
    struct arguments args;
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

        ret = val_host_rmi_rec_create(args.rd, args.rec,
                                    args.params_ptr);
        if (ret != PACK_CODE(test_data[i].status, test_data[i].index))
        {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n", 0, 0);

    ret = val_host_rmi_rec_create(c_args.rd_valid, c_args.rec_valid,
                                  c_args.params_valid);

    /* Keep track of the REC for destruction */
    realm_test[REALM_VALID].rec[0] = c_args.rec_valid;
    /* Valid call should give success if footprints have not changed */
    if (ret != 0)
    {
        LOG(TEST, "\t\nERROR\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }


    LOG(TEST, "\n\tNegative Observability Check\n", 0, 0);

    ret = val_host_rmi_rec_create(c_args.rd_valid, c_args.rec_valid,
                                   c_args.params_valid);
    /* Now the command should fail with the same parameters (footprint has changed) */
    if (ret == 0) {
        LOG(ERROR, "\n\tNegative Observability Check Failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
