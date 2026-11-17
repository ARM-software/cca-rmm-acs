/*
 * Copyright (c) 2023, 2025-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_command.h"

#define IPA_ADDR_UNASSIGNED 0x0
#define RIM_WORDS 8U

static uint64_t rec_create(uint64_t rd)
{
    /* Delegate granule for the REC */
    uint64_t rec = val_host_delegate_granule();
    val_host_rec_params_ts *params;
    uint64_t ret;

    if (rec == VAL_ERROR)
        return VAL_ERROR;

    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (params == NULL)
        return VAL_ERROR;

    val_memset(params, 0, PAGE_SIZE);

    /* Populate params structure */
    params->pc = 0;
    params->flags = RMI_RUNNABLE;
    params->mpidr = 1;


    /* Create the REC */
    ret = val_host_rmi_rec_create(rd, rec, (uint64_t)params);
    val_host_mem_free(params);
    if (ret) {
        LOG(ERROR, " REC create failed \n");
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
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    /* Create rec */
    if (rec_create(realm1.rd))
    {
        LOG(ERROR, "REC create failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    /* Add data granule */
    if (create_mapping(IPA_ADDR_UNASSIGNED, true, realm1.rd))
    {
        LOG(ERROR, "Couldn't create the assigned protected mapping\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    uint64_t data1 = val_host_delegate_granule();
    if (data1 == VAL_ERROR)
    {
        LOG(ERROR, " Delegation failed \n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto destroy_realm;
    }

    uint64_t src1 = val_host_undelegate_granule();
    if (src1 == VAL_ERROR)
    {
        LOG(ERROR, " Memory allocation failed \n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto destroy_realm;
    }

    val_memset((void *)src1, 0x0, PAGE_SIZE);

    uint64_t flags1 = RMI_MEASURE_CONTENT;

    if (val_host_rmi_rtt_data_map_init(realm1.rd, data1, IPA_ADDR_UNASSIGNED, src1, flags1))
    {
        LOG(ERROR, "Data create failed.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    /* Activate the realm */
    ret = val_host_rmi_realm_activate(realm1.rd);
    if (ret)
    {
        LOG(ERROR, "Realm activate failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    ret = val_host_rmi_rec_enter(realm1.rec[0], realm1.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    rec_exit1 = &(((val_host_rec_run_ts *)realm1.run[0])->exit);
    if (rec_exit1->exit_reason != RMI_EXIT_HOST_CALL ||
        rec_exit1->imm != VAL_SWITCH_TO_HOST || rec_exit1->gprs[0] != 0U)
    {
        LOG(ERROR, "Realm 1 did not return a successful RIM read\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    val_host_realm_ts realm2;

    val_memset(&realm2, 0, sizeof(realm2));

    val_host_realm_params(&realm2);

    realm2.vmid = 1;

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm2, 0))
    {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    /* Add data granule */
    if (create_mapping(IPA_ADDR_UNASSIGNED, true, realm2.rd))
    {
        LOG(ERROR, "Couldn't create the assigned protected mapping\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }

    uint64_t data2 = val_host_delegate_granule();
    if (data2 == VAL_ERROR)
    {
        LOG(ERROR, " Delegation failed \n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto destroy_realm;
    }

    uint64_t src2 = val_host_undelegate_granule();
    if (src2 == VAL_ERROR)
    {
        LOG(ERROR, " Memory allocation failed \n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto destroy_realm;
    }

    val_memset((void *)src2, 0x0, PAGE_SIZE);

    uint64_t flags2 = RMI_MEASURE_CONTENT;

    if (val_host_rmi_rtt_data_map_init(realm2.rd, data2, IPA_ADDR_UNASSIGNED, src2, flags2))
    {
        LOG(ERROR, "Data create failed.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_realm;
    }

    /* Create rec */
    if (rec_create(realm2.rd))
    {
        LOG(ERROR, "REC create failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto destroy_realm;
    }

    /* Activate the realm */
    ret = val_host_rmi_realm_activate(realm2.rd);
    if (ret)
    {
        LOG(ERROR, "Realm activate failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_realm;
    }

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm2.rec[0], realm2.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto destroy_realm;
    }

    rec_exit2 = &(((val_host_rec_run_ts *)realm2.run[0])->exit);
    if (rec_exit2->exit_reason != RMI_EXIT_HOST_CALL ||
        rec_exit2->imm != VAL_SWITCH_TO_HOST || rec_exit2->gprs[0] != 0U)
    {
        LOG(ERROR, "Realm 2 did not return a successful RIM read\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto destroy_realm;
    }

    /* Compare the full Realm measurement returned by RSI_MEASUREMENT_READ. */
    if (!val_memcmp(&rec_exit1->gprs[1], &rec_exit2->gprs[1],
                    RIM_WORDS * sizeof(rec_exit1->gprs[0]))) {
        LOG(ERROR, " RIM values are same \n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto destroy_realm;
    }
    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_realm:
    return;
}
