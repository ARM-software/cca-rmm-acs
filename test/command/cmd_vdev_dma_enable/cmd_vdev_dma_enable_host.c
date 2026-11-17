/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_pcie.h"
#include "command_common_host.h"

static uint64_t prepare_new_vdev(val_host_pdev_ts *pdev_dev,
                                 val_host_realm_ts *realm,
                                 val_host_vdev_ts *vdev_dev)
{
    val_host_rmi_vdev_params_ts *params;
    val_host_dev_comm_data_ts *comm_data;
    val_smc_param_ts args;
    uint64_t vdev, req_addr, rsp_addr, ret;

    vdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!vdev)
        return VAL_TEST_PREP_SEQ_FAILED;

    ret = val_host_rmi_granule_range_delegate(vdev, vdev + PAGE_SIZE);
    if (ret)
        goto free_vdev;

    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!params)
        goto undelegate_vdev;

    comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!comm_data)
        goto free_params;

    req_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!req_addr)
        goto free_comm_data;

    rsp_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!rsp_addr)
        goto free_req_addr;

    val_memset(comm_data, 0, PAGE_SIZE);
    comm_data->enter.req_addr = req_addr;
    comm_data->enter.rsp_addr = rsp_addr;

    val_memset(params, 0, PAGE_SIZE);
    params->vdev_id = 1U;
    params->tdi_id = pdev_dev->rid_base + 1U;
    params->num_addr_range = 1U;
    params->addr_range[0].base =
        pdev_dev->ncoh_addr_range[0].base + PAGE_SIZE;
    params->addr_range[0].top =
        params->addr_range[0].base + PAGE_SIZE;

    ret = val_host_rmi_vdev_create(realm->rd, pdev_dev->pdev, vdev,
                                   (uint64_t)params);
    if (ret)
        goto free_rsp_addr;
    val_host_mem_free(params);

    vdev_dev->vdev = vdev;
    vdev_dev->vdev_id = 1U;
    vdev_dev->tdi_id = pdev_dev->rid_base + 1U;
    vdev_dev->pdev = pdev_dev->pdev;
    vdev_dev->realm_rd = realm->rd;
    vdev_dev->dev_comm_data = comm_data;

    args = val_host_rmi_vdev_get_state(vdev);
    if (args.x0 || (args.x1 != RMI_VDEV_NEW))
        return VAL_TEST_PREP_SEQ_FAILED;

    return VAL_SUCCESS;

free_rsp_addr:
    val_host_mem_free((void *)rsp_addr);
free_req_addr:
    val_host_mem_free((void *)req_addr);
free_comm_data:
    val_host_mem_free(comm_data);
free_params:
    val_host_mem_free(params);
undelegate_vdev:
    (void)val_host_rmi_granule_range_undelegate(vdev, vdev + PAGE_SIZE);
free_vdev:
    val_host_mem_free((void *)vdev);
    return VAL_TEST_PREP_SEQ_FAILED;
}

void cmd_vdev_dma_enable_host(void)
{
    val_host_pdev_ts pdev_dev;
    val_host_vdev_ts vdev_dev;
    val_host_vdev_ts new_vdev;
    val_host_realm_ts realm;
    val_host_rec_exit_ts *rec_exit;
    val_host_rec_enter_ts *rec_enter;
    uint64_t ret;
    uint32_t num_bdf, rp_bdf, i;

    if (!val_host_rmm_supports_da())
    {
        LOG(ALWAYS, "DA feature not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    val_memset(&pdev_dev, 0, sizeof(pdev_dev));
    val_memset(&vdev_dev, 0, sizeof(vdev_dev));
    val_memset(&new_vdev, 0, sizeof(new_vdev));
    val_memset(&realm, 0, sizeof(realm));

    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR)
        return;

    if (val_pcie_find_doe_capability(&num_bdf, &pdev_dev.bdf,
                                     &pdev_dev.doe_cap_base))
        return;

    if (val_pcie_get_rootport(pdev_dev.bdf, &rp_bdf) != 0U)
        return;

    pdev_dev.root_id = (uint16_t)rp_bdf;
    pdev_dev.rid_base = PCIE_CREATE_BDF_PACKED(pdev_dev.bdf);
    pdev_dev.rid_top = pdev_dev.rid_base + 2U;
    val_host_set_pdev_flags(&pdev_dev.pdev_flags);

    val_host_get_addr_range(&pdev_dev);
    if ((pdev_dev.ncoh_num_addr_range == 0U) ||
        (pdev_dev.ncoh_addr_range[0].top <=
         pdev_dev.ncoh_addr_range[0].base) ||
        ((pdev_dev.ncoh_addr_range[0].top -
          pdev_dev.ncoh_addr_range[0].base) < (2U * PAGE_SIZE)))
    {
        LOG(ERROR, "PDEV has no range large enough for two VDEVs\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_device;
    }

    vdev_dev.num_addr_range = 1U;
    vdev_dev.tdi_id = pdev_dev.rid_base;
    vdev_dev.addr_range[0].base = pdev_dev.ncoh_addr_range[0].base;
    vdev_dev.addr_range[0].top =
        vdev_dev.addr_range[0].base + PAGE_SIZE;

    ret = g_vdev_state_prep_sequence(&pdev_dev, &vdev_dev, &realm,
                                     RMI_VDEV_STARTED);
    if (ret == VAL_TEST_PREP_SEQ_FAILED)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_device;
    }

    if (prepare_new_vdev(&pdev_dev, &realm, &new_vdev))
    {
        LOG(ERROR, "Second VDEV did not reach VDEV_NEW\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_device;
    }

    rec_enter = &(((val_host_rec_run_ts *)realm.run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);

    for (i = 0U; i < 2U; i++)
    {
        ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
        if (ret || (rec_exit->exit_reason != RMI_EXIT_HOST_CALL))
        {
            LOG(ERROR, "Initial REC entry did not exit for a Host call\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_device;
        }
    }

    rec_enter->gprs[1] = vdev_dev.vdev_id;
    rec_enter->gprs[2] = new_vdev.vdev_id;

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret || (rec_exit->exit_reason != RMI_EXIT_HOST_CALL))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }
    if (!IS_TEST_START(val_get_status()))
        goto destroy_device;

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret || (rec_exit->exit_reason != RMI_EXIT_HOST_CALL))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_device;
    }
    if (IS_TEST_START(val_get_status()))
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));

destroy_device:
    if (new_vdev.vdev &&
        val_host_vdev_teardown(&realm, &pdev_dev, &new_vdev))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        return;
    }

    if (vdev_dev.vdev && (vdev_dev.vdev != VAL_TEST_PREP_SEQ_FAILED) &&
        val_host_vdev_teardown(&realm, &pdev_dev, &vdev_dev))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        return;
    }

    if (pdev_dev.pdev && (pdev_dev.pdev != VAL_TEST_PREP_SEQ_FAILED) &&
        val_host_pdev_teardown(&pdev_dev, pdev_dev.pdev))
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
}
