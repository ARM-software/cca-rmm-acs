/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_rec_enter_data.h"
#include "command_common_host.h"
#include "val_host_helpers.h"

#define IPA_WIDTH 40
#define MAX_GRANULES 256

#define NUM_RECS 1

#define NUM_REALMS 5
#define VALID_REALM 0
#define NEW_REALM 1
#define NEW_REALM_2 2
#define NOT_RUNNABLE_REALM 3
#define RUNNABLE_REALM 4

#define IPA_ADDR_DATA 0
#define TEST_IPA 0x10000

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
        LOG(ERROR, "Realm create failed\n");
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
        LOG(ERROR, "Couldn't create/add rec_valid\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    uint64_t rec = realm_test[VALID_REALM].rec[mpidr];

    if (val_host_rmi_realm_activate(rd)) {
        LOG(ERROR, "Couldn't activate the Realm\n");
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
        LOG(ERROR, "Couldn't create/add the rec\n");
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
        LOG(ERROR, "Couldn't create/add the rec\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (val_host_rmi_realm_activate(rd)) {
        LOG(ERROR, "Couldn't activate the Realm\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm_test[NOT_RUNNABLE_REALM].rec[mpidr];
}

static uint64_t emulated_mmio_prep_sequence(void)
{
    val_host_rec_enter_ts *run_ptr = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    /* Clear the structure entirely */
    val_memset(run_ptr, 0x0, PAGE_SIZE);

    run_ptr->flags = 0x1;

    return (uint64_t)run_ptr;
}

static uint64_t gicv3_invalid_prep_sequence(void)
{
    val_host_rec_enter_ts *run_ptr = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

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
        LOG(ERROR, " REC create failed \n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return params->aux[0];
}

static uint64_t g_rec_non_emulatable_abort_prep_sequence(void)
{
    uint64_t ret, phys;
    val_host_rec_exit_ts *rec_exit = NULL;
    val_host_rec_enter_ts *rec_enter = NULL;

    val_memset(&realm_test[RUNNABLE_REALM], 0, sizeof(realm_test[RUNNABLE_REALM]));

    val_host_realm_params(&realm_test[RUNNABLE_REALM]);

    realm_test[RUNNABLE_REALM].vmid = RUNNABLE_REALM;
    realm_test[RUNNABLE_REALM].rec_count = 2;

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm_test[RUNNABLE_REALM], false))
    {
        LOG(ERROR, "Realm setup failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Prepare IPA whose HIPAS = UNASIGNED, RIPAS = RAM */
    if (val_host_ripas_init(&realm_test[RUNNABLE_REALM], TEST_IPA, TEST_IPA + PAGE_SIZE,
                                                     VAL_RTT_MAX_LEVEL, PAGE_SIZE))
    {
            LOG(ERROR, "RMI_INIT_RIPAS failed \n");
            return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Activate realm */
    if (val_host_realm_activate(&realm_test[RUNNABLE_REALM]))
    {
        LOG(ERROR, "Realm activate failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    rec_enter = &(((val_host_rec_run_ts *)realm_test[RUNNABLE_REALM].run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm_test[RUNNABLE_REALM].run[0])->exit);

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm_test[RUNNABLE_REALM].rec[0],
                                 realm_test[RUNNABLE_REALM].run[0]);
    if (ret) {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Check that REC Exit was due to host call because of realm requesting for test IPA */
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "Unexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Return the test IPA to realm */
    rec_enter->gprs[1] = TEST_IPA;

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm_test[RUNNABLE_REALM].rec[0],
                                 realm_test[RUNNABLE_REALM].run[0]);
    if (ret) {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Check that REC exit was due to Data Abort due to realm access to IPA whose
     * HIPAS,RIPAS = UNASSIGNED,RAM */
    if (validate_rec_exit_da(rec_exit, TEST_IPA, ESR_ISS_DFSC_TTF_L3,
                                NON_EMULATABLE_DA, ESR_WnR_WRITE))
    {
        LOG(ERROR, "REC exit DA: params mismatch\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Fix the Abort */
    phys = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!phys)
    {
        LOG(ERROR, "val_host_mem_alloc failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    ret = val_host_map_protected_data_unknown(&realm_test[RUNNABLE_REALM], phys,
                                                                TEST_IPA, PAGE_SIZE);
    if (ret)
    {
        LOG(ERROR, "DATA_CREATE_UNKNOWN failed, ret = %d \n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm_test[RUNNABLE_REALM].rec[0];
}

static uint64_t g_rec_psci_pending_prep_sequence(void)
{
    uint64_t ret;

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm_test[RUNNABLE_REALM].rec[0],
                                 realm_test[RUNNABLE_REALM].run[0]);
    if (ret) {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm_test[RUNNABLE_REALM].rec[0];
}

static uint64_t g_rec_owner_state_system_off_prep_sequence(void)
{
    uint64_t ret;

    /* Complete pending PSCI request*/
    ret = val_host_rmi_psci_complete(realm_test[RUNNABLE_REALM].rec[0],
                                     realm_test[RUNNABLE_REALM].rec[1], PSCI_E_SUCCESS);
    if (ret)
    {
        LOG(ERROR, " PSCI_COMPLETE Failed, ret=%x\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Enter REC[1]  */
    ret = val_host_rmi_rec_enter(realm_test[RUNNABLE_REALM].rec[1],
                                 realm_test[RUNNABLE_REALM].run[1]);
    if (ret)
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);

    return realm_test[RUNNABLE_REALM].rec[1];
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
            args->rec = g_rec_non_emulatable_abort_prep_sequence();
            if (args->rec == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->run_ptr = emulated_mmio_prep_sequence();
            break;

        case REC_PSCI_PENDING:
            args->rec = g_rec_psci_pending_prep_sequence();
            args->run_ptr = c_args.run_ptr_valid;
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
            LOG(ERROR, "Unknown intent label encountered\n");
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
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
              i + 1, test_data[i].msg, test_data[i].label);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        ret = val_host_rmi_rec_enter(args.rec, args.run_ptr);

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "Test Failure!The ABI call returned: %xExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto exit;
        }
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
