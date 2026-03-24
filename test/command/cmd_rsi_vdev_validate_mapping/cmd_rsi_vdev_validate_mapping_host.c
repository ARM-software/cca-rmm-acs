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
#include "val_host_helpers.h"

static uint32_t num_bdf;

/*
 * Verify all REC exit fields are zero except those explicitly allowed
 * (VDEV mapping exits, gicv3_*, cnt*, and pmu_ovf_status).
 * Return VAL_ERROR if any unexpected non-zero value is found.
 */
static uint32_t validate_rec_exit_zero_fields(val_host_rec_exit_ts *rec_exit)
{
    val_host_rec_exit_ts exit_copy;
    val_host_rec_exit_ts expected_zero_exit;
    uint8_t *exit_bytes;
    size_t i;
    uint64_t value;

    val_memcpy(&exit_copy, rec_exit, sizeof(exit_copy));
    val_memset(&expected_zero_exit, 0, sizeof(expected_zero_exit));

    exit_copy.exit_reason = 0;
    exit_copy.dev_mem_base = 0;
    exit_copy.dev_mem_top = 0;
    exit_copy.dev_mem_pa = 0;
    exit_copy.vdev_id_1 = 0;

    exit_copy.gicv3_hcr = 0;
    val_memset(exit_copy.gicv3_lrs, 0, sizeof(exit_copy.gicv3_lrs));
    exit_copy.gicv3_misr = 0;
    exit_copy.gicv3_vmcr = 0;

    exit_copy.cntp_ctl = 0;
    exit_copy.cntp_cval = 0;
    exit_copy.cntv_ctl = 0;
    exit_copy.cntv_cval = 0;

    exit_copy.pmu_ovf_status = 0;

    if (!val_memcmp(&exit_copy, &expected_zero_exit, sizeof(exit_copy))) {
        return VAL_SUCCESS;
    }

    exit_bytes = (uint8_t *)&exit_copy;
    for (i = 0; i + sizeof(uint64_t) <= sizeof(exit_copy); i += sizeof(uint64_t)) {
        val_memcpy(&value, exit_bytes + i, sizeof(uint64_t));
        if (value) {
            LOG(ERROR, "Unexpected REC exit word offset 0x%lx\n",
                (unsigned long)i);
            return VAL_ERROR;
        }
    }

    return VAL_ERROR;
}

void cmd_rsi_vdev_validate_mapping_host(void)
{
    val_host_pdev_ts pdev_dev;
    val_host_vdev_ts vdev_dev;
    val_host_realm_ts realm;
    val_host_rec_exit_ts *rec_exit = NULL;
    val_host_rec_enter_ts *rec_enter = NULL;
    uint64_t ret;
    val_smc_param_ts args;
    uint32_t rp_bdf, status;
    val_host_pdev_flags_ts pdev_flags;
    uint8_t *shared_report_buff = (val_get_shared_region_base() + TEST_USE_OFFSET4);
    uint64_t report_size;

    /* Skip if RMM do not support DA */
    if (!val_host_rmm_supports_da())
    {
        LOG(ALWAYS, "DA feature not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto exit;
    }

    val_memset(&pdev_dev, 0, sizeof(pdev_dev));
    val_memset(&vdev_dev, 0, sizeof(vdev_dev));
    val_memset(&realm, 0, sizeof(realm));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR)
        goto exit;

    if (val_pcie_find_doe_capability(&num_bdf, (uint32_t *)&pdev_dev.bdf,
                                                     &pdev_dev.doe_cap_base))
        goto exit;

    status = val_pcie_get_rootport((uint32_t)pdev_dev.bdf, &rp_bdf);
    if (status != 0)
        goto exit;

    pdev_dev.root_id = (uint16_t)rp_bdf;

    val_host_set_pdev_flags(&pdev_flags);
    val_memcpy(&pdev_dev.pdev_flags, &pdev_flags, sizeof(pdev_flags));

    vdev_dev.vdev = g_vdev_state_prep_sequence(&pdev_dev, &vdev_dev, &realm, RMI_VDEV_STARTED);
    if (vdev_dev.vdev == VAL_TEST_PREP_SEQ_FAILED)
        goto destroy_device;

    rec_enter = &(((val_host_rec_run_ts *)realm.run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_device;
    }

    /* Check that REC Exit was due to host call*/
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "Unexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_device;
    }

    /* Enter realm.rec[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_device;
    }

    /* Check that REC Exit was due to host call*/
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "Unexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    report_size = vdev_dev.ifc_report_len;
    val_memcpy(shared_report_buff, vdev_dev.ifc_report, report_size);

    rec_enter->gprs[1] = vdev_dev.vdev_id;
    rec_enter->gprs[4] = report_size;

    do {
        ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
        if (ret)
        {
            LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto destroy_device;
        }

        if (rec_exit->exit_reason == RMI_EXIT_VDEV_REQUEST) {
            args = val_host_rmi_vdev_complete(realm.rec[0], vdev_dev.vdev);
            if (args.x0)
            {
                LOG(ERROR, "VDEV complete failed ret %x\n", args.x0);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
                goto destroy_device;
            }
        } else if (rec_exit->exit_reason == RMI_EXIT_VDEV_MAP) {
            if (validate_rec_exit_zero_fields(rec_exit)) {
                LOG(ERROR, "REC exit contains unexpected non-zero fields\n");
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
                goto destroy_device;
            }

            if (rec_exit->vdev_id_1 != vdev_dev.vdev_id) {
                LOG(ERROR, "Unexpected vdev_id_1: 0x%lx\n", rec_exit->vdev_id_1);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
                goto destroy_device;
            }

            args.x0 = val_host_dev_mem_map(&realm, vdev_dev.vdev, rec_exit->dev_mem_base,
                                           rec_exit->dev_mem_top, rec_exit->dev_mem_pa);
            if (args.x0)
            {
                LOG(ERROR, "val_host_dev_mem_map failed \n");
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
                goto destroy_device;
            }

            args = val_host_rmi_vdev_validate_mapping(realm.rd, realm.rec[0], pdev_dev.pdev,
                        vdev_dev.vdev, rec_exit->dev_mem_base, rec_exit->dev_mem_top);
            if (args.x0)
            {
                LOG(ERROR, "RMI_VDEV_VALIDATE_MAPPING failed with ret = 0x%x\n", args.x0);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
                goto destroy_device;
            }
        } else if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
            LOG(ERROR, "Unexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason,
                rec_exit->esr);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
            goto destroy_device;
        }
    } while (rec_exit->exit_reason != RMI_EXIT_HOST_CALL);

    /* All failure conditions checks are done,
     * enter realm again for positive observability check
     */
    do {
        ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
        if (ret)
        {
            LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
            goto destroy_device;
        }

        if (rec_exit->exit_reason == RMI_EXIT_VDEV_REQUEST) {
            args = val_host_rmi_vdev_complete(realm.rec[0], vdev_dev.vdev);
            if (args.x0)
            {
                LOG(ERROR, "VDEV complete failed ret %x\n", args.x0);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
                goto destroy_device;
            }
        } else if (rec_exit->exit_reason == RMI_EXIT_VDEV_MAP) {
            if (validate_rec_exit_zero_fields(rec_exit)) {
                LOG(ERROR, "REC exit contains unexpected non-zero fields\n");
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(14)));
                goto destroy_device;
            }

            if (rec_exit->vdev_id_1 != vdev_dev.vdev_id) {
                LOG(ERROR, "Unexpected vdev_id_1: 0x%lx\n", rec_exit->vdev_id_1);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(15)));
                goto destroy_device;
            }

            args.x0 = val_host_dev_mem_map(&realm, vdev_dev.vdev, rec_exit->dev_mem_base,
                                           rec_exit->dev_mem_top, rec_exit->dev_mem_pa);
            if (args.x0)
            {
                LOG(ERROR, "val_host_dev_mem_map failed \n");
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(16)));
                goto destroy_device;
            }

            args = val_host_rmi_vdev_validate_mapping(realm.rd, realm.rec[0], pdev_dev.pdev,
                        vdev_dev.vdev, rec_exit->dev_mem_base, rec_exit->dev_mem_top);
            if (args.x0)
            {
                LOG(ERROR, "RMI_VDEV_VALIDATE_MAPPING failed with ret = 0x%x\n", args.x0);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(17)));
                goto destroy_device;
            }
        } else if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
            LOG(ERROR, "Unexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason,
                rec_exit->esr);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(18)));
            goto destroy_device;
        }
    } while (rec_exit->exit_reason != RMI_EXIT_HOST_CALL);

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    if (vdev_dev.vdev &&
        val_host_vdev_teardown(&realm, &pdev_dev, &vdev_dev))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(19)));
        goto exit;
    }

    if (pdev_dev.pdev &&
        val_host_pdev_teardown(&pdev_dev, pdev_dev.pdev))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(20)));
        goto exit;
    }

exit:
    return;
}
