/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_rhi.h"

static uint64_t rhi_da_step(val_host_realm_ts *realm,
                            val_host_pdev_ts *pdev_obj,
                            val_host_vdev_ts *vdev_obj)
{
    uint64_t ret;

    ret = val_host_rmi_rec_enter(realm->rec[0], realm->run[0]);
    if (ret)
    {
        LOG(ERROR, "REC enter failed, ret=%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return VAL_ERROR;
    } else if (val_host_check_realm_exit_rsi_host_call((val_host_rec_run_ts *)realm->run[0]))
    {
        LOG(ERROR, "REC_EXIT: HOST_CALL params mismatch\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return VAL_ERROR;
    }

    ret = val_host_rhi_da_dispatch(realm, pdev_obj, vdev_obj);
    if (ret)
    {
        LOG(ERROR, "RHI command failed, ret=%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void rhi_da_object_read_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;
    val_host_rec_enter_ts *rec_enter = NULL;
    val_host_pdev_ts pdev_obj;
    val_host_vdev_ts vdev_obj[2];
    uint64_t i;
    struct step_desc {
        const char *desc;
    };
    const struct step_desc step_desc[] = {
        {"invalid vdev id"},
        {"invalid object type"},
        {"unsupported object type"},
        {"invalid buffer IPA"},
        {"invalid offset"},
        {"valid object read"}
    };

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

    for (i = 0; i < (sizeof(step_desc) / sizeof(step_desc[0])); i++)
    {
        LOG(ALWAYS, "RHI_DA_OBJECT_READ: %s\n", step_desc[i].desc);
        ret = rhi_da_step(&realm, &pdev_obj, vdev_obj);
        if (ret)
            goto destroy_device;
    }

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "REC enter failed, ret=%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_device;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    if (val_host_vdev_teardown(&realm, &pdev_obj, vdev_obj))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }

    if (val_host_pdev_teardown(&pdev_obj, pdev_obj.pdev))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_realm;
    }

    /* Free test resources */
destroy_realm:
    return;
}
