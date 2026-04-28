/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_rhi.h"

enum rhi_da_step_result {
    RHI_DA_STEP_DONE = 0,
    RHI_DA_STEP_MORE = 1,
    RHI_DA_STEP_ERROR = 2,
};

static enum rhi_da_step_result rhi_da_step(val_host_realm_ts *realm,
                                           val_host_pdev_ts *pdev_obj,
                                           val_host_vdev_ts *vdev_obj)
{
    uint64_t ret;

    ret = val_host_rmi_rec_enter(realm->rec[0], realm->run[0]);
    if (ret)
    {
        LOG(ERROR, "REC enter failed, ret=%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return RHI_DA_STEP_ERROR;
    }

    if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm->run[0])
        == VAL_SUCCESS) {
        return RHI_DA_STEP_DONE;
    }

    if (val_host_check_realm_exit_rsi_host_call((val_host_rec_run_ts *)realm->run[0]))
    {
        LOG(ERROR, "REC_EXIT: HOST_CALL params mismatch\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return RHI_DA_STEP_ERROR;
    }

    ret = val_host_rhi_da_dispatch(realm, pdev_obj, vdev_obj);
    if (ret)
    {
        LOG(ERROR, "RHI command failed, ret=%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        return RHI_DA_STEP_ERROR;
    }

    return RHI_DA_STEP_MORE;
}

void rhi_da_vdev_get_interface_report_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;
    val_host_rec_enter_ts *rec_enter = NULL;
    val_host_pdev_ts pdev_obj;
    val_host_vdev_ts vdev_obj[2];
    uint64_t rc;

    val_memset(&realm, 0, sizeof(realm));
    val_memset(&pdev_obj, 0, sizeof(pdev_obj));
    val_memset(vdev_obj, 0, sizeof(vdev_obj));
    ret = da_init(&realm, &pdev_obj, vdev_obj, RMI_VDEV_LOCKED);
    if (ret)
    {
        LOG(ERROR, "DA init failed, ret=%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    /* REC enter REC[0] execution */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_device;
    } else if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "REC_EXIT: HOST_CALL params mismatch\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_device;
    }

    rec_enter = &(((val_host_rec_run_ts *)realm.run[0])->enter);
    rec_enter->gprs[1] = vdev_obj[0].vdev_id;

    for (;;)
    {
        rc = rhi_da_step(&realm, &pdev_obj, vdev_obj);
        if (rc == RHI_DA_STEP_DONE)
            break;
        if (rc == RHI_DA_STEP_ERROR)
            goto destroy_device;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    if (val_host_vdev_teardown(&realm, &pdev_obj, vdev_obj))
    {
        LOG(ERROR, "VDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    if (val_host_pdev_teardown(&pdev_obj, pdev_obj.pdev))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }

    /* Free test resources */
destroy_realm:
    return;
}
