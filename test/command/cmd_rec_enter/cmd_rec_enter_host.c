/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_rec_enter_data.h"
#include "command_common_host.h"

#define IPA_WIDTH 40
#define MAX_GRANULES 256

#define NUM_RECS 1

#define NUM_REALMS 5
#define VALID_REALM 0
#define NEW_REALM 1
#define NEW_REALM_2 2
#define NOT_RUNNABLE_REALM 3
#define SYSTEM_OFF_REALM 4

#define IPA_ADDR_DATA 0

static val_host_realm_ts realm_test[NUM_REALMS];

static struct argument_store {
    uint64_t rec_valid;
    uint64_t run_ptr_valid;
} c_args;

static struct invalid_arguments {
    uint64_t rd_gran;
    uint64_t rec_not_runnable;
    uint64_t rec_owner_new;
    uint64_t rec_owner_system_off;
} c_args_invalid;

struct arguments {
    uint64_t rec;
    uint64_t run_ptr;
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

static uint64_t add_rec(uint64_t entry, uint64_t flags, uint64_t mpidr, uint64_t vmid)
{
    /* Delegate granule for the REC */
    uint64_t rec = g_delegated_prep_sequence();
    if (rec == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    /* Track for reuse/destruction */
    realm_test[vmid].rec[mpidr] = rec;

    val_host_rec_params_ts *params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    /* Zero out the gprs */
    uint64_t i;

    for (i = 0; i < (sizeof(params->gprs) / sizeof(params->gprs[0])); i++)
        params->gprs[i] = 0x0;

    /* Set PC to the entry point */
    params->pc = entry;
    /* Set the runnable flag */
    params->flags = flags;
    params->mpidr = mpidr;
    /* Get aux granules count */
    uint64_t aux_count;
    /*  Fetch the rd of the Realm */
    uint64_t rd = realm_test[vmid].rd;

    if (val_host_rmi_rec_aux_count(rd, &aux_count))
        return VAL_TEST_PREP_SEQ_FAILED;

    params->num_aux = aux_count;

    /* Create all aux granules */
    for (i = 0; i < aux_count; i++) {
        uint64_t aux_rec = g_delegated_prep_sequence();
        if (aux_rec == VAL_TEST_PREP_SEQ_FAILED)
            return VAL_TEST_PREP_SEQ_FAILED;

        params->aux[i] = aux_rec;
    }

    /* Create the REC */
    if (val_host_rmi_rec_create(rd, rec, (uint64_t)params))
        return VAL_TEST_PREP_SEQ_FAILED;

    return VAL_SUCCESS;
}

static uint64_t rec_valid_prep_sequence(void)
{

    uint64_t rd = g_rd_new_prep_sequence(VALID_REALM);
    if (rd == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    uint64_t entry_addr = 0x0;
    uint64_t mpidr = 0;

    if (add_rec(entry_addr, RMI_RUNNABLE, mpidr, VALID_REALM)) {
        LOG(ERROR, "\tCouldn't create/add rec_valid\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    uint64_t rec = realm_test[VALID_REALM].rec[mpidr];

    if (val_host_rmi_realm_activate(rd)) {
        LOG(ERROR, "\tCouldn't activate the Realm\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return rec;
}

static uint64_t run_ptr_valid_prep_sequence(void)
{
    /* Allocate a granule for RunParams */
    struct rec_entry *params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    /* Clear the structure entirely */
    val_memset(params, 0x0, PAGE_SIZE);

    return (uint64_t)params;
}

static uint64_t g_rec_ready_owner_state_new_prep_sequence(void)
{
    if (g_rd_new_prep_sequence(NEW_REALM_2) == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    uint64_t entry_addr = 0x0;
    uint64_t mpidr = 0;

    if (add_rec(entry_addr, RMI_RUNNABLE, mpidr, NEW_REALM_2)) {
        LOG(ERROR, "\tCouldn't create/add the rec\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm_test[NEW_REALM_2].rec[mpidr];
}

static uint64_t g_rec_ready_not_runnable_prep_sequence(void)
{

    uint64_t rd = g_rd_new_prep_sequence(NOT_RUNNABLE_REALM);
    if (rd == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    uint64_t entry_addr = 0x0;
    uint64_t mpidr = 0;

    if (add_rec(entry_addr, RMI_NOT_RUNNABLE, mpidr, NOT_RUNNABLE_REALM)) {
        LOG(ERROR, "\tCouldn't create/add the rec\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (val_host_rmi_realm_activate(rd)) {
        LOG(ERROR, "\tCouldn't activate the Realm\n", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm_test[NOT_RUNNABLE_REALM].rec[mpidr];
}

static uint64_t emulated_mmio_prep_sequence(void)
{
    val_host_rec_entry_ts *run_ptr = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    /* Clear the structure entirely */
    val_memset(run_ptr, 0x0, PAGE_SIZE);

    run_ptr->flags = 0x1;

    return (uint64_t)run_ptr;
}

static uint64_t gicv3_invalid_prep_sequence(void)
{
    val_host_rec_entry_ts *run_ptr = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    /* Clear the structure entirely */
    val_memset(run_ptr, 0x0, PAGE_SIZE);

    run_ptr->gicv3_lrs[0] = 3ULL << 61;

    return (uint64_t)run_ptr;
}

static uint64_t g_rec_aux_prep_sequence(void)
{
    /* Delegate granule for the REC */
    uint64_t rec = g_delegated_prep_sequence();
    if (rec == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    /* Track for reuse/destruction */
    realm_test[NEW_REALM].rec[1] = rec;

    val_host_rec_params_ts *params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    /* Zero out the gprs */
    uint64_t i;

    for (i = 0; i < (sizeof(params->gprs) / sizeof(params->gprs[0])); i++)
        params->gprs[i] = 0x0;

    /* Set PC to the entry point */
    params->pc = 0;
    /* Set the runnable flag */
    params->flags = RMI_RUNNABLE;
    params->mpidr = 0;
    /* Get aux granules count */
    uint64_t aux_count;
    /* Fetch the rd of the Realm */
    uint64_t rd = realm_test[NEW_REALM].rd;

    if (val_host_rmi_rec_aux_count(rd, &aux_count))
        return VAL_TEST_PREP_SEQ_FAILED;

    params->num_aux = aux_count;

    /* Create all aux granules */
    for (i = 0; i < aux_count; i++) {
        uint64_t aux_rec = g_delegated_prep_sequence();
        if (aux_rec == VAL_TEST_PREP_SEQ_FAILED)
            return VAL_TEST_PREP_SEQ_FAILED;

        params->aux[i] = aux_rec;
    }

    /* Create the REC */
    if (val_host_rmi_rec_create(rd, rec, (uint64_t)params)) {
        LOG(ERROR, "\n\t REC create failed ", 0, 0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return params->aux[0];
}

static uint64_t g_rec_owner_state_system_off_prep_sequence(void)
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

    return realm_test[SYSTEM_OFF_REALM].rec[0];

}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.rec_valid = rec_valid_prep_sequence();
    if (c_args.rec_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.run_ptr_valid = run_ptr_valid_prep_sequence();

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case RUN_PTR_UNALIGNED:
            args->rec = c_args.rec_valid;
            args->run_ptr = g_unaligned_prep_sequence(c_args.run_ptr_valid);
            break;

        case RUN_PTR_DEV_MEM:
            args->rec = c_args.rec_valid;
            args->run_ptr = g_dev_mem_prep_sequence();
            break;

        case RUN_PTR_OUSIDE_OF_PERMITTED_PA:
            args->rec = c_args.rec_valid;
            args->run_ptr = g_outside_of_permitted_pa_prep_sequence();
            break;

        case RUN_PTR_PAS_REALM:
            args->rec = c_args.rec_valid;
            args->run_ptr = g_delegated_prep_sequence();
            if (args->run_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case RUN_PTR_PAS_SECURE:
            args->rec = c_args.rec_valid;
            args->run_ptr = g_secure_prep_sequence();
            break;

        case REC_UNALIGNED:
            args->rec = g_unaligned_prep_sequence(c_args.rec_valid);
            args->run_ptr = c_args.run_ptr_valid;
            break;

        case REC_DEV_MEM:
            args->rec = g_dev_mem_prep_sequence();
            args->run_ptr = c_args.run_ptr_valid;
            break;

        case REC_OUTSIDE_OF_PERMITTED_PA:
            args->rec = g_outside_of_permitted_pa_prep_sequence();
            args->run_ptr = c_args.run_ptr_valid;
            break;

        case REC_GRAN_STATE_UNDELEGATED:
            args->rec = g_undelegated_prep_sequence();
            if (args->rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->run_ptr = c_args.run_ptr_valid;
            break;

        case REC_GRAN_STATE_DELEGATED:
            args->rec = g_delegated_prep_sequence();
            if (args->rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->run_ptr = c_args.run_ptr_valid;
            break;

        case REC_GRAN_STATE_RD:
            args->rec = realm_test[VALID_REALM].rd;
            args->run_ptr = c_args.run_ptr_valid;
            break;

        case REC_GRAN_STATE_RTT:
            args->rec = realm_test[VALID_REALM].rtt_l0_addr;
            args->run_ptr = c_args.run_ptr_valid;
            break;

        case REC_GRAN_STATE_DATA:
            c_args_invalid.rd_gran = g_rd_new_prep_sequence(NEW_REALM);
            if (c_args_invalid.rd_gran == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->rec = g_data_prep_sequence(c_args_invalid.rd_gran, IPA_ADDR_DATA);
            if (args->rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->run_ptr = c_args.run_ptr_valid;
            break;

        case REC_GRAN_STATE_REC_AUX:
            args->rec = g_rec_aux_prep_sequence();
            if (args->rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->run_ptr = c_args.run_ptr_valid;
            break;

        case REALM_NEW:
            args->rec = g_rec_ready_owner_state_new_prep_sequence();
            if (args->rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            c_args_invalid.rec_owner_new = args->rec;
            args->run_ptr = c_args.run_ptr_valid;
            break;

        case REALM_SYSTEM_OFF:
            args->rec = g_rec_owner_state_system_off_prep_sequence();
            if (args->rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            c_args_invalid.rec_owner_system_off = args->rec;
            args->run_ptr = c_args.run_ptr_valid;
            break;

        case REC_NOT_RUNNABLE:
            args->rec = g_rec_ready_not_runnable_prep_sequence();
            if (args->rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            c_args_invalid.rec_not_runnable = args->rec;
            args->run_ptr = c_args.run_ptr_valid;
            break;

        case REC_EMULATED_MMIO:
            args->rec = c_args.rec_valid;
            args->run_ptr = emulated_mmio_prep_sequence();
            break;

        case RUN_PTR_INVALID_GIV3_HCR:
            args->rec = c_args.rec_valid;
            args->run_ptr = gicv3_invalid_prep_sequence();
            break;

        case REC_UNALIGNED_INVALID_GICV3:
            args->rec = g_unaligned_prep_sequence(c_args.rec_valid);
            args->run_ptr = gicv3_invalid_prep_sequence();
            break;

        case REC_DEV_MEM_INVALID_GICV3:
            args->rec = g_dev_mem_prep_sequence();
            args->run_ptr = gicv3_invalid_prep_sequence();
            break;

        case REC_GRAN_STATE_UNDELEGATED_INVALID_GICV3:
            args->rec = g_undelegated_prep_sequence();
            if (args->rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->run_ptr = gicv3_invalid_prep_sequence();
            break;

        case RUN_PAS_SECURE_REC_NOT_RUNNABLE:
            args->rec = c_args_invalid.rec_not_runnable;
            args->run_ptr = g_secure_prep_sequence();
            break;

        case RUN_PAS_SECURE_REALM_NEW:
            args->rec = c_args_invalid.rec_owner_new;
            args->run_ptr = g_secure_prep_sequence();
            break;

        case RUN_PAS_SECURE_REALM_SYSTEM_OFF:
            args->rec = c_args_invalid.rec_owner_system_off;
            args->run_ptr = g_secure_prep_sequence();
            break;

        case RUN_DEV_MEM_REC_NOT_RUNNABLE:
            args->rec = c_args_invalid.rec_not_runnable;
            args->run_ptr = g_dev_mem_prep_sequence();
            break;

        case RUN_DEV_MEM_REALM_NEW:
            args->rec = c_args_invalid.rec_owner_new;
            args->run_ptr = g_dev_mem_prep_sequence();
            break;

        case RUN_DEV_MEM_REALM_SYSTEM_OFF:
            args->rec = c_args_invalid.rec_owner_system_off;
            args->run_ptr = g_dev_mem_prep_sequence();
            break;

        default:
            /* set status to failure */
            LOG(ERROR, "\t\nUnknown intent label encountered\n", 0, 0);
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_rec_enter_host(void)
{
    uint64_t ret;
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

        ret = val_host_rmi_rec_enter(args.rec, args.run_ptr);

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto exit;
        }
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
