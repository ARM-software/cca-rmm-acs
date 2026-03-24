/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_vdev_complete_data.h"
#include "command_common_host.h"
#include "val_host_pcie.h"

static uint32_t num_bdf;

static struct argument_store {
    uint64_t rec_ptr_valid;
    uint64_t vdev_ptr_valid;
    uint64_t rec_run_ptr;
    uint64_t rec_ptr_pending_none_state;
    uint64_t pdev_ptr_valid;
    val_host_vdev_ts vdev_dev_valid;
    val_host_pdev_ts pdev_dev_valid;
    val_host_realm_ts realm_valid;
} c_args;

static struct invalid_argument_store {
    uint64_t rec_ptr;
    uint64_t vdev_ptr;
    uint64_t rec_run_ptr;
    uint64_t pdev_ptr_invalid;
    val_host_vdev_ts vdev_dev_invalid;
    val_host_pdev_ts pdev_dev_invalid;
    val_host_realm_ts realm_invalid;
} c_args_invalid;

struct arguments {
    uint64_t rec_ptr;
    uint64_t vdev_ptr;
};

uint64_t invalid_vdev_id_vdev_ptr;

static uint64_t invalid_rec_owner_prep_sequence(void)
{
    val_host_pdev_ts pdev_dev;
    val_host_vdev_ts vdev_dev;
    val_host_realm_ts realm;

    if (val_pcie_find_doe_capability(&num_bdf, (uint32_t *)&pdev_dev.bdf,
                                                     &pdev_dev.doe_cap_base))
        return VAL_TEST_PREP_SEQ_FAILED;

    vdev_dev.vmid = 1;
    vdev_dev.vdev_id = 1;
    vdev_dev.vdev = g_vdev_new_prep_sequence(&pdev_dev, &vdev_dev, &realm);
    if (vdev_dev.vdev == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args_invalid.rec_ptr = realm.rec[0];
    c_args_invalid.vdev_ptr = vdev_dev.vdev;
    c_args_invalid.pdev_ptr_invalid = pdev_dev.pdev;
    c_args_invalid.pdev_dev_invalid = pdev_dev;
    c_args_invalid.vdev_dev_invalid = vdev_dev;
    c_args_invalid.realm_invalid = realm;

    return VAL_SUCCESS;
}

static uint64_t invalid_vdev_id_prep_sequence(void)
{
    uint64_t vdev, ret, j;
    val_host_vdev_params_ts *vdev_params;
    val_smc_param_ts args;
    val_host_pdev_flags_ts pdev_flags;
    val_host_vdev_flags_ts vdev_flags;
    uint64_t flags_pdev, flags_vdev, aux_count;

    val_memset(&pdev_flags, 0, sizeof(pdev_flags));
    val_memset(&vdev_flags, 0, sizeof(vdev_flags));

    pdev_flags.spdm = RMI_SPDM_TRUE;
    pdev_flags.ncoh_ide = RMI_IDE_TRUE;
    vdev_flags.vsmmu = RMI_FEATURE_FALSE;

    val_memcpy(&flags_pdev, &pdev_flags, sizeof(pdev_flags));
    val_memcpy(&flags_vdev, &vdev_flags, sizeof(vdev_flags));

    /* Create VDEV */
    /* Allocate and delegate VDEV */
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

    /* Allocate memory for vdev params */
    vdev_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    if (vdev_params == NULL)
    {
        LOG(ERROR, "Failed to allocate memory for vdev_params.\n");
        goto exit;
    }
    val_memset(vdev_params, 0, PAGE_SIZE_4K);

    /* Populate params */
    vdev_params->flags = flags_vdev;
    vdev_params->vdev_id = 2;
    vdev_params->tdi_id = 0;
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
                LOG(ERROR, "vdev delegation failed, vdev=0x%lx, ret=0x%lx.\n", vdev_params->aux[j],
                                                                               ret);
                goto exit;
            }
        }
    }

    /* Create vdev */
    args = val_host_rmi_vdev_create(c_args.realm_valid.rd, c_args.pdev_ptr_valid, vdev,
                                                          (uint64_t)vdev_params);
    if (args.x0)
    {
        LOG(ERROR, "VDEV create failed.\n");
        goto exit;
    }

    args = val_host_rmi_vdev_get_state(vdev);
    if (args.x0)
    {

        LOG(ERROR, "VDEV get state failed ret %lx.\n", args.x0);
        goto exit;
    }

    /* Check VDEV state is RMI_VDEV_NEW */
    if (args.x1 != RMI_VDEV_NEW)
    {
        LOG(ERROR, "VDEV state should be RMI_VDEV_NEW, ret %lx.\n", args.x1);
        goto exit;
    }

    invalid_vdev_id_vdev_ptr = vdev;

    return invalid_vdev_id_vdev_ptr;

exit:
    return VAL_TEST_PREP_SEQ_FAILED;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    val_host_pdev_ts pdev_dev;
    val_host_vdev_ts vdev_dev;
    val_host_rec_exit_ts *rec_exit = NULL;
    val_host_rec_enter_ts *rec_enter = NULL;
    uint64_t ret;
    val_host_realm_ts realm;
    uint32_t rp_bdf, status;
    val_host_pdev_flags_ts pdev_flags;

    val_memset(&pdev_dev, 0, sizeof(pdev_dev));
    val_memset(&vdev_dev, 0, sizeof(vdev_dev));
    val_memset(&realm, 0, sizeof(realm));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR)
        return VAL_TEST_PREP_SEQ_FAILED;

    if (val_pcie_find_doe_capability(&num_bdf, (uint32_t *)&pdev_dev.bdf,
                                                     &pdev_dev.doe_cap_base))
        return VAL_TEST_PREP_SEQ_FAILED;

    status = val_pcie_get_rootport((uint32_t)pdev_dev.bdf, &rp_bdf);
    if (status != 0)
        return VAL_TEST_PREP_SEQ_FAILED;

    pdev_dev.root_id = (uint16_t)rp_bdf;

    val_host_set_pdev_flags(&pdev_flags);
    val_memcpy(&pdev_dev.pdev_flags, &pdev_flags, sizeof(pdev_flags));

    vdev_dev.rec_count = 2;
    vdev_dev.vdev = g_vdev_state_prep_sequence(&pdev_dev, &vdev_dev, &realm, RMI_VDEV_STARTED);
    if (vdev_dev.vdev == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    rec_enter = &(((val_host_rec_run_ts *)realm.run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%lx\n", ret);
        goto exit;
    }

    /* Check that REC Exit was due to host call*/
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "\tUnexpected REC exit, %lx. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        goto exit;
    }

    rec_enter->gprs[1] = vdev_dev.vdev_id;

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%lx\n", ret);
        goto exit;
    }

    /* Check that REC Exit was due to host call*/
    if (rec_exit->exit_reason != RMI_EXIT_VDEV_REQUEST) {
        LOG(ERROR, "\tUnexpected REC exit, %lx. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        goto exit;
    }

    c_args.rec_ptr_valid = realm.rec[0];
    c_args.vdev_ptr_valid = vdev_dev.vdev;
    c_args.rec_run_ptr = realm.run[0];
    c_args.rec_ptr_pending_none_state = realm.rec[1];
    c_args.pdev_ptr_valid = pdev_dev.pdev;
    c_args.pdev_dev_valid = pdev_dev;
    c_args.vdev_dev_valid = vdev_dev;
    c_args.realm_valid = realm;

    return VAL_SUCCESS;

exit:
    return VAL_TEST_PREP_SEQ_FAILED;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case REC_UNALIGNED:
            args->rec_ptr = g_unaligned_prep_sequence(c_args.rec_ptr_valid);
            args->vdev_ptr = c_args.vdev_ptr_valid;
            break;

        case REC_OUTSIDE_OF_PERMITTED_PA:
            args->rec_ptr = g_outside_of_permitted_pa_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            break;

        case REC_DEV_MEM_MMIO:
            args->rec_ptr = g_dev_mem_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            break;

        case REC_GRAN_STATE_UNDELEGATED:
            args->rec_ptr = g_undelegated_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            break;

        case VDEV_UNALIGNED:
            args->rec_ptr = c_args.rec_ptr_valid;
            args->vdev_ptr = g_unaligned_prep_sequence(c_args.vdev_ptr_valid);
            break;

        case VDEV_OUTSIDE_OF_PERMITTED_PA:
            args->rec_ptr = c_args.rec_ptr_valid;
            args->vdev_ptr = g_outside_of_permitted_pa_prep_sequence();
            break;

        case VDEV_DEV_MEM_MMIO:
            args->rec_ptr = c_args.rec_ptr_valid;
            args->vdev_ptr = g_dev_mem_prep_sequence();
            break;

        case VDEV_GRAN_STATE_UNDELEGATED:
            args->rec_ptr = c_args.rec_ptr_valid;
            args->vdev_ptr = g_undelegated_prep_sequence();
            break;

        case NOT_REC_PENDING_VDEV_REQUEST:
            args->rec_ptr = c_args.rec_ptr_pending_none_state;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            break;

        case INVALID_REC_OWNER:
            if (invalid_rec_owner_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->rec_ptr = c_args_invalid.rec_ptr;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            break;

        case INVALID_VDEV_ID:
            args->rec_ptr = c_args.rec_ptr_valid;
            args->vdev_ptr = invalid_vdev_id_prep_sequence();
            if (args->vdev_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_TEST_PREP_SEQ_FAILED;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_vdev_complete_host(void)
{
    uint64_t i;
    val_smc_param_ts cmd_ret;
    struct arguments args;

    /* Skip if RMM do not support DA */
    if (!val_host_rmm_supports_da())
    {
        LOG(ALWAYS, "DA feature not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto exit;
    }

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_device;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "\n\tCheck %d : ", i + 1);
        LOG(TEST, test_data[i].msg);
        LOG(TEST, "; intent id : 0x%x \n", test_data[i].label);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto destroy_device;
        }

        cmd_ret = val_host_rmi_vdev_complete(args.rec_ptr, args.vdev_ptr);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_device;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n");
    cmd_ret = val_host_rmi_vdev_complete(c_args.rec_ptr_valid, c_args.vdev_ptr_valid);
    if (cmd_ret.x0 != 0)
    {
        LOG(ERROR, "Command failed. %lx\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    /* VDEV complete should failed again as REC is in REC_PENDING_NONE state*/
    cmd_ret = val_host_rmi_vdev_complete(c_args.rec_ptr_valid, c_args.vdev_ptr_valid);
    if (cmd_ret.x0 != RMI_ERROR_INPUT)
    {
        LOG(ERROR, "VDEV complete failed ret %lx.\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_device;
    }

    cmd_ret = val_host_rmi_vdev_destroy(c_args.realm_valid.rd, c_args.pdev_ptr_valid,
                                                     invalid_vdev_id_vdev_ptr);
    if (cmd_ret.x0 != 0)
    {
        LOG(ERROR, "VDEV destroy failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_device;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    if (c_args.vdev_ptr_valid &&
        val_host_vdev_teardown(&c_args.realm_valid, &c_args.pdev_dev_valid,
                               &c_args.vdev_dev_valid))
    {
        LOG(ERROR, "VDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    if (c_args.pdev_ptr_valid &&
        val_host_pdev_teardown(&c_args.pdev_dev_valid, c_args.pdev_ptr_valid))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

    if (c_args_invalid.pdev_ptr_invalid &&
        val_host_vdev_teardown(&c_args_invalid.realm_invalid, &c_args_invalid.pdev_dev_invalid,
                               &c_args_invalid.vdev_dev_invalid))
    {
        LOG(ERROR, "VDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto exit;
    }

    if (c_args_invalid.pdev_ptr_invalid &&
        val_host_pdev_teardown(&c_args_invalid.pdev_dev_invalid, c_args_invalid.pdev_ptr_invalid))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto exit;
    }

exit:
    return;
}
