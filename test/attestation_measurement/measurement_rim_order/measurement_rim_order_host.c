/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_command.h"

#define IPA_ADDR_UNASSIGNED 0x0

static uint64_t rec_create(uint64_t rd)
{
    /* Delegate granule for the REC */
    uint64_t rec = val_host_delegate_granule();
    if (rec == VAL_ERROR)
        return VAL_ERROR;

    val_host_rec_params_ts *params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    uint64_t i;

    for (i = 0; i < (sizeof(params->gprs) / sizeof(params->gprs[0])); i++)
        params->gprs[i] = 0x0;

    /* Populate params structure */
    params->pc = 0;
    params->flags = RMI_RUNNABLE;
    params->mpidr = 1;

    uint64_t aux_count;

    if (val_host_rmi_rec_aux_count(rd, &aux_count))
        return VAL_TEST_PREP_SEQ_FAILED;

    params->num_aux = aux_count;

    /* Create all aux granules */
    for (i = 0; i < aux_count; i++) {
        uint64_t aux_rec = val_host_delegate_granule();
        if (aux_rec == VAL_ERROR)
            return VAL_ERROR;

        params->aux[i] = aux_rec;
    }

    /* Create the REC */
    if (val_host_rmi_rec_create(rd, rec, (uint64_t)params)) {
        LOG(ERROR, "\n\t REC create failed ", 0, 0);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void measurement_rim_order_host(void)
{
    val_host_realm_ts realm1;
    uint64_t ret;
    val_host_rec_exit_ts *rec_exit1 = NULL;
    val_host_rec_exit_ts *rec_exit2 = NULL;

    val_memset(&realm1, 0, sizeof(realm1));

    val_host_realm_params(&realm1);

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm1, 0))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    /* Create rec */
    if (rec_create(realm1.rd))
    {
        LOG(ERROR, "\tREC create failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    /* Add data granule */
    if (create_mapping(IPA_ADDR_UNASSIGNED, true, realm1.rd))
    {
        LOG(ERROR, "\tCouldn't create the assigned protected mapping\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
    }

    uint64_t data1 = val_host_delegate_granule();
    if (data1 == VAL_ERROR)
    {
        LOG(ERROR, "\t Delegation failed \n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto destroy_realm;
    }

    uint64_t src1 = val_host_undelegate_granule();
    if (src1 == VAL_ERROR)
    {
        LOG(ERROR, "\t Memory allocation failed \n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto destroy_realm;
    }

    val_memset((void *)src1, 0x0, PAGE_SIZE);

    uint64_t flags1 = RMI_MEASURE_CONTENT;

    if (val_host_rmi_data_create(realm1.rd, data1, IPA_ADDR_UNASSIGNED, src1, flags1))
    {
        LOG(ERROR, "\tData create failed.\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    /* Activate the realm */
    ret = val_host_rmi_realm_activate(realm1.rd);
    if (ret)
    {
        LOG(ERROR, "\tRealm activate failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    ret = val_host_rmi_rec_enter(realm1.rec[0], realm1.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    rec_exit1 = &(((val_host_rec_run_ts *)realm1.run[0])->exit);

    val_host_realm_ts realm2;

    val_memset(&realm2, 0, sizeof(realm2));

    val_host_realm_params(&realm2);

    realm2.vmid = 1;

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm2, 0))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    /* Add data granule */
    if (create_mapping(IPA_ADDR_UNASSIGNED, true, realm2.rd))
    {
        LOG(ERROR, "\tCouldn't create the assigned protected mapping\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }

    uint64_t data2 = val_host_delegate_granule();
    if (data1 == VAL_ERROR)
    {
        LOG(ERROR, "\t Delegation failed \n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto destroy_realm;
    }

    uint64_t src2 = val_host_undelegate_granule();
    if (src2 == VAL_ERROR)
    {
        LOG(ERROR, "\t Memory allocation failed \n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto destroy_realm;
    }

    val_memset((void *)src2, 0x0, PAGE_SIZE);

    uint64_t flags2 = RMI_MEASURE_CONTENT;

    if (val_host_rmi_data_create(realm2.rd, data2, IPA_ADDR_UNASSIGNED, src2, flags2))
    {
        LOG(ERROR, "\tData create failed.\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_realm;
    }

    /* Create rec */
    if (rec_create(realm2.rd))
    {
        LOG(ERROR, "\tREC create failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto destroy_realm;
    }

    /* Activate the realm */
    ret = val_host_rmi_realm_activate(realm2.rd);
    if (ret)
    {
        LOG(ERROR, "\tRealm activate failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_realm;
    }

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm2.rec[0], realm2.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto destroy_realm;
    }

    rec_exit2 = &(((val_host_rec_run_ts *)realm2.run[0])->exit);

    /* Compare command output with zero initialized structure */
    if (!val_memcmp(rec_exit1->gprs, rec_exit2->gprs, sizeof(rec_exit1->gprs))) {
        LOG(ERROR, "\t RIM values are same \n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto destroy_realm;
    }
    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_realm:
    return;
}
