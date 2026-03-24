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

static uint32_t num_bdf;

void cmd_vdev_get_info_host(void)
{
    val_host_pdev_ts pdev_dev;
    val_host_vdev_ts vdev_dev;
    val_host_realm_ts realm;
    val_host_rec_exit_ts *rec_exit = NULL;
    val_host_rec_enter_ts *rec_enter = NULL;
    uint64_t ret;
    val_smc_param_ts args;
    uint64_t out_top;
    uint32_t rp_bdf, status;
    val_host_pdev_flags_ts pdev_flags;

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
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_device;
    }

    /* Check that REC Exit was due to host call*/
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_device;
    }

    /* Enter realm.rec[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_device;
    }

    /* Check that REC Exit was due to host call*/
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    rec_enter->gprs[1] = vdev_dev.vdev_id;

    do {
        ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
        if (ret)
        {
            LOG(ERROR, "\tRec enter failed, ret=%x\n", ret);
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
        } else if (rec_exit->exit_reason == RMI_EXIT_RIPAS_CHANGE) {
            ret = val_host_rmi_rtt_set_ripas(realm.rd, realm.rec[0], rec_exit->ripas_base,
                                            rec_exit->ripas_top, &out_top);
            if (ret)
            {
                LOG(ERROR, "\tRMI_RTT_SET_RIPAS failed, ret=%x\n", ret);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
                goto destroy_device;
            }
        } else if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
            LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason,
                rec_exit->esr);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
            goto destroy_device;
        }
    } while (rec_exit->exit_reason != RMI_EXIT_HOST_CALL);

    /* All failure conditions checks are done,
     * enter realm again for positive observability check
     */
    /* Enter realm.rec[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_device;
    }

    if (rec_exit->exit_reason == RMI_EXIT_VDEV_REQUEST) {
        args = val_host_rmi_vdev_complete(realm.rec[0], vdev_dev.vdev);
        if (args.x0)
        {
            LOG(ERROR, "VDEV complete failed ret %x\n", args.x0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
            goto destroy_device;
        }
    }

    /* Enter realm.rec[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_device;
    }

    /* Check that REC Exit was due to host call*/
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason,
                                                                    rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto destroy_device;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    if (vdev_dev.vdev &&
        val_host_vdev_teardown(&realm, &pdev_dev, &vdev_dev))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto exit;
    }

    if (pdev_dev.pdev &&
        val_host_pdev_teardown(&pdev_dev, pdev_dev.pdev))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(14)));
        goto exit;
    }

exit:
    return;
}
