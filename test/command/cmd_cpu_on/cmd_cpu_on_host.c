/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "command_common_host.h"

#define REC_RUNNABLE         1
#define REC_NOT_RUNNABLE     2

static val_host_realm_ts realm;

static uint64_t g_rec_not_runnable_prep_sequence(void)
{
    /* Delegate granule for the REC */
    uint64_t rec = g_delegated_prep_sequence();
    if (rec == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    /* Track for reuse/destruction */
    realm.rec[REC_NOT_RUNNABLE] = rec;

    val_host_rec_params_ts *params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    uint64_t i;

    for (i = 0; i < (sizeof(params->gprs) / sizeof(params->gprs[0])); i++)
        params->gprs[i] = 0x0;

    /* Populate params structure */
    params->pc = 0;
    params->flags = RMI_NOT_RUNNABLE;
    params->mpidr = REC_NOT_RUNNABLE;

    uint64_t aux_count;
    uint64_t rd = realm.rd;

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

    return VAL_SUCCESS;
}


void cmd_cpu_on_host(void)
{
    uint64_t ret;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    realm.rec_count = 2;

    /* Populate realm with two RECs*/
    if (val_host_realm_setup(&realm, 0))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    /* Add a NOT_RUNNABLE REC */
    if (g_rec_not_runnable_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED)
    {
        LOG(ERROR, "\tREC not runnable creation failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    /* Activate the realm */
    ret = val_host_rmi_realm_activate(realm.rd);
    if (ret)
    {
        LOG(ERROR, "\tRealm activate failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    /* Check that REC exit was due to PSCI_CPU_ON */
    if (val_host_check_realm_exit_psci((val_host_rec_run_ts *)realm.run[0],
                                PSCI_CPU_ON_AARCH64))
    {
        LOG(ERROR, "\tREC exit not due to PSCI_CPU_ON\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    /* Complete pending PSCI by returning a RUNNABLE target REC*/
    ret = val_host_rmi_psci_complete(realm.rec[0], realm.rec[REC_RUNNABLE]);
    if (ret)
    {
        LOG(ERROR, "\t PSCI_COMPLETE Failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    /* Enter REC[0] For failure condition check*/
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    /* Check that REC exit was due to PSCI_CPU_ON */
    if (val_host_check_realm_exit_psci((val_host_rec_run_ts *)realm.run[0],
                                PSCI_CPU_ON_AARCH64))
    {
        LOG(ERROR, "\tREC exit not due to PSCI_CPU_ON\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }

    /* Complete pending PSCI by returning a NOT_RUNNABLE target REC */
    ret = val_host_rmi_psci_complete(realm.rec[0], realm.rec[REC_NOT_RUNNABLE]);
    if (ret)
    {
        LOG(ERROR, "\tPSCI_COMPLETE Failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_realm;
    }

    /* Enter REC[0] for positive observability check  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto destroy_realm;
    }

    /* Check that REC exit was due to PSCI_CPU_ON */
    if (val_host_check_realm_exit_psci((val_host_rec_run_ts *)realm.run[0],
                                PSCI_CPU_ON_AARCH64))
    {
        LOG(ERROR, "\tREC exit not due to PSCI_CPU_ON\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_realm;
    }

    /* Complete pending PSCI by returning a NOT_RUNNABLE target REC */
    ret = val_host_rmi_psci_complete(realm.rec[0], realm.rec[REC_NOT_RUNNABLE]);
    if (ret)
    {
        LOG(ERROR, "\tPSCI_COMPLETE Failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto destroy_realm;
    }

    /* Enter REC[0] to continue positive observability check*/
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
