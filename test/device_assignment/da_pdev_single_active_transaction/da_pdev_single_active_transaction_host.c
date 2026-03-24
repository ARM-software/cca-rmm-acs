/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "command_common_host.h"
#include "val_host_pcie.h"
#include "val_host_helpers.h"
#include "val_host_alloc.h"
#include "val_host_da.h"

#define VALID_REALM   1

static uint32_t num_bdf;

static uint64_t create_locked_vdev1(val_host_pdev_ts *pdev_dev,
                                    val_host_vdev_ts *vdev_dev,
                                    val_host_realm_ts *realm)
{
    uint32_t rp_bdf, status;
    val_host_pdev_flags_ts pdev_flags;

    val_memset(pdev_dev, 0, sizeof(*pdev_dev));
    val_memset(vdev_dev, 0, sizeof(*vdev_dev));
    val_memset(realm, 0, sizeof(*realm));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR)
        return VAL_TEST_PREP_SEQ_FAILED;

    if (val_pcie_find_doe_capability(&num_bdf, (uint32_t *)&pdev_dev->bdf,
                                                     &pdev_dev->doe_cap_base))
        return VAL_TEST_PREP_SEQ_FAILED;

    status = val_pcie_get_rootport((uint32_t)pdev_dev->bdf, &rp_bdf);
    if (status != 0)
        return VAL_TEST_PREP_SEQ_FAILED;

    pdev_dev->root_id = (uint16_t)rp_bdf;

    val_host_set_pdev_flags(&pdev_flags);
    val_memcpy(&pdev_dev->pdev_flags, &pdev_flags, sizeof(pdev_flags));

    vdev_dev->vmid = VALID_REALM;
    vdev_dev->vdev_id = 1;
    vdev_dev->rec_count = 1;

    vdev_dev->vdev = g_vdev_state_prep_sequence(pdev_dev, vdev_dev, realm, RMI_VDEV_LOCKED);
    if (vdev_dev->vdev == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    return VAL_SUCCESS;
}

static uint64_t create_locked_vdev2(val_host_realm_ts *realm,
                                    val_host_pdev_ts *pdev_dev,
                                    val_host_vdev_ts *vdev_dev,
                                    uint32_t vdev_id)
{
    uint64_t vdev, ret, j;
    val_host_vdev_params_ts *vdev_params;
    val_smc_param_ts args;
    val_host_pdev_flags_ts pdev_flags;
    val_host_vdev_flags_ts vdev_flags;
    uint64_t flags_pdev, flags_vdev, aux_count;

    val_memset(vdev_dev, 0, sizeof(*vdev_dev));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));
    val_memset(&vdev_flags, 0, sizeof(vdev_flags));

    pdev_flags.spdm = RMI_SPDM_TRUE;
    pdev_flags.ncoh_ide = RMI_IDE_TRUE;
    vdev_flags.vsmmu = RMI_FEATURE_FALSE;

    val_memcpy(&flags_pdev, &pdev_flags, sizeof(pdev_flags));
    val_memcpy(&flags_vdev, &vdev_flags, sizeof(vdev_flags));

    vdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!vdev)
    {
        LOG(ERROR, "Failed to allocate memory for vdev.\n");
        goto exit;
    } else {
        ret = val_host_rmi_granule_delegate(vdev);
        if (ret)
        {
            LOG(ERROR, "vdev delegation failed, vdev=0x%lx, ret=0x%lx.\n", vdev, ret);
            goto exit;
        }
    }

    args = val_host_rmi_vdev_aux_count(flags_pdev, flags_vdev);
    if (args.x0) {
        LOG(ERROR, "VDEV AUX count ABI failed, ret value is: %lx.\n", args.x0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    aux_count = args.x1;

    vdev_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (vdev_params == NULL)
    {
        LOG(ERROR, "Failed to allocate memory for vdev_params.\n");
        goto exit;
    }
    val_memset(vdev_params, 0, PAGE_SIZE_4K);

    vdev_params->flags = flags_vdev;
    vdev_params->vdev_id = vdev_id;
    vdev_params->tdi_id = 1;
    vdev_params->num_aux = aux_count;

    for (j = 0; j < vdev_params->num_aux; j++)
    {
        vdev_params->aux[j] = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
        if (!vdev_params->aux[j])
        {
            LOG(ERROR, "Failed to allocate memory for aux vdev.\n");
            goto exit;
        }
        else
        {
            ret = val_host_rmi_granule_delegate(vdev_params->aux[j]);
            if (ret)
            {
                LOG(ERROR, "vdev delegation failed, vdev=0x%lx, ret=0x%lx.\n",
                    vdev_params->aux[j], ret);
                goto exit;
            }
        }
    }

    args = val_host_rmi_vdev_create(realm->rd, pdev_dev->pdev, vdev,
                                    (uint64_t)vdev_params);
    if (args.x0)
    {
        LOG(ERROR, "Vdev create failed.\n");
        goto exit;
    }

    args = val_host_rmi_vdev_get_state(vdev);
    if (args.x0)
    {
        LOG(ERROR, "Vdev get state failed ret %lx.\n", args.x0);
        goto exit;
    }

    if (args.x1 != RMI_VDEV_NEW)
    {
        LOG(ERROR, "Vdev state should be RMI_VDEV_NEW, ret %lx.\n", args.x1);
        goto exit;
    }

    vdev_dev->vdev = vdev;
    vdev_dev->vdev_id = vdev_id;
    vdev_dev->realm_rd = realm->rd;
    vdev_dev->pdev = pdev_dev->pdev;
    vdev_dev->dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!vdev_dev->dev_comm_data)
        goto exit;

    vdev_dev->dev_comm_data->enter.req_addr = pdev_dev->dev_comm_data->enter.req_addr;
    vdev_dev->dev_comm_data->enter.resp_addr = pdev_dev->dev_comm_data->enter.resp_addr;

    if (val_host_dev_communicate(realm, pdev_dev, vdev_dev, RMI_VDEV_UNLOCKED))
        goto exit;

    args = val_host_rmi_vdev_lock(realm->rd, vdev_dev->pdev, vdev_dev->vdev);
    if (args.x0)
    {
        LOG(ERROR, "\tVDEV_LOCK failed, ret=%x\n", args.x0);
        goto exit;
    }

    if (val_host_dev_communicate(realm, pdev_dev, vdev_dev, RMI_VDEV_LOCKED))
        goto exit;

    vdev_dev->meas = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_VDEV_MEAS_LEN_MAX);
    if (vdev_dev->meas == NULL)
        goto exit;
    vdev_dev->meas_len = 0;

    vdev_dev->ifc_report = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_VDEV_IFC_REPORT_LEN_MAX);
    if (vdev_dev->ifc_report == NULL)
        goto exit;
    vdev_dev->ifc_report_len = 0;

    return VAL_SUCCESS;

exit:
    return VAL_TEST_PREP_SEQ_FAILED;
}

void da_pdev_single_active_transaction_host(void)
{
    val_host_realm_ts realm;
    val_host_pdev_ts pdev_dev;
    val_host_vdev_ts vdev1;
    val_host_vdev_ts vdev2;
    val_host_vdev_measure_params_ts *vdev_measure_params;
    val_smc_param_ts cmd_ret;
    uint32_t vdev_id;

    val_memset(&realm, 0, sizeof(realm));
    val_memset(&pdev_dev, 0, sizeof(pdev_dev));
    val_memset(&vdev1, 0, sizeof(vdev1));
    val_memset(&vdev2, 0, sizeof(vdev2));

    if (create_locked_vdev1(&pdev_dev, &vdev1, &realm) == VAL_TEST_PREP_SEQ_FAILED)
    {
        LOG(ERROR, "VDEV1 creation failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    vdev_id = 2;
    if (create_locked_vdev2(&realm, &pdev_dev, &vdev2, vdev_id) == VAL_TEST_PREP_SEQ_FAILED)
    {
        LOG(ERROR, "VDEV2 creation failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_vdev1;
    }

    vdev_measure_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (vdev_measure_params == NULL)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_vdev2;
    }
    val_memset(vdev_measure_params, 0, PAGE_SIZE);
    vdev_measure_params->indices[31] = (unsigned char)1U << 6U;

    cmd_ret = val_host_rmi_vdev_get_measurements(realm.rd, pdev_dev.pdev,
                                                 vdev1.vdev, (uint64_t)vdev_measure_params);
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "\n\tVdev1 GET_MEASUREMENTS failed. %x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_vdev2;
    }

    if (val_host_dev_communicate(&realm, &pdev_dev, &vdev1, RMI_VDEV_LOCKED))
    {
        LOG(ERROR, "VDEV communicate failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_vdev2;
    }

    cmd_ret = val_host_rmi_vdev_get_measurements(realm.rd, pdev_dev.pdev,
                                                 vdev2.vdev, (uint64_t)vdev_measure_params);
    if (cmd_ret.x0 == RMI_SUCCESS)
    {
        LOG(ERROR, "\n\tSecond GET_MEASUREMENTS unexpectedly succeeded. %x\n",
            cmd_ret.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_vdev2;
    }

    if (val_host_dev_communicate(&realm, &pdev_dev, &vdev2, RMI_VDEV_LOCKED))
    {
        LOG(ERROR, "VDEV communicate failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_vdev2;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_vdev2:
    if (val_host_vdev_teardown(&realm, &pdev_dev, &vdev2))
    {
        LOG(ERROR, "VDEV2 teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_vdev1;
    }

destroy_vdev1:
    if (val_host_vdev_teardown(&realm, &pdev_dev, &vdev1))
    {
        LOG(ERROR, "VDEV1 teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
    }

    if (val_host_pdev_teardown(&pdev_dev, pdev_dev.pdev))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
    }

destroy_realm:
    return;
}
