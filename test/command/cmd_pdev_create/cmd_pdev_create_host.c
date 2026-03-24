/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_pdev_create_data.h"
#include "command_common_host.h"
#include "val_host_pcie.h"

static uint32_t num_bdf;

static struct argument_store {
    uint64_t pdev_ptr_valid;
    uint64_t params_ptr_valid;
    val_host_pdev_ts pdev_dev_valid;
} c_args;

struct arguments {
    uint64_t pdev_ptr;
    uint64_t params_ptr;
};

typedef enum {
    PARAMS_VALID = 0x0,
    INVALID_DEVICE_ID,
    USED_DEVICE_ID,
    INVALID_ROOT_PORT_ID,
    INVALID_RID_RANGE,
    RID_RANGE_OVERLAPPED,
    PARAM_BASE_ADDRESS_RANGE_UNALIGNED,
    PARAM_TOP_ADDRESS_RANGE_UNALIGNED,
    ADDRESS_INVALID_RANGE,
    OVERLAPPED_ADDRESS_RANGE_WITHIN_PDEV,
    OVERLAPPED_ADDRESS_RANGE_ANOTHER_PDEV,
    INVALID_NUM_AUX,
    AUX_GRAN_UNALINED,
    PARAMS_AUX_ALIASED,
    PARAMS_AUX_UNDELEGATED,
    INVALID_NCOH_IDE_SID
} prep_seq_type;

static uint64_t pdev_new_prep_sequence(void)
{
    val_host_pdev_ts pdev_dev;
    val_host_pdev_flags_ts pdev_flags;

    val_memset(&pdev_dev, 0, sizeof(pdev_dev));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

    if (val_pcie_find_doe_capability(&num_bdf, &pdev_dev.bdf,
                                                     &pdev_dev.doe_cap_base))
        return VAL_TEST_PREP_SEQ_FAILED;

    val_host_set_pdev_flags(&pdev_flags);
    val_memcpy(&pdev_dev.pdev_flags, &pdev_flags, sizeof(pdev_flags));

    pdev_dev.pdev = g_pdev_new_prep_sequence(&pdev_dev);
    if (c_args.pdev_ptr_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    return pdev_dev.pdev;
}

static uint64_t params_prep_sequence(prep_seq_type type)
{
    val_smc_param_ts args;
    val_host_pdev_params_ts *pdev_params;
    val_host_pdev_flags_ts pdev_flags;
    uint64_t flags, aux_count, i;
    val_host_pdev_ts pdev_dev;
    uint32_t rp_bdf, status;

    val_memset(&pdev_dev, 0, sizeof(pdev_dev));

    /* Allocate memory for params */
    pdev_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev_params == NULL)
    {
        LOG(ERROR, "Failed to allocate memory for pdev_params.\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    val_memset(pdev_params, 0, PAGE_SIZE_4K);
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

    pdev_flags.spdm = RMI_SPDM_TRUE;
    pdev_flags.ncoh_ide = RMI_IDE_TRUE;

    val_memcpy(&flags, &pdev_flags, sizeof(pdev_flags));

    /* Get aux granules count */
    args = val_host_rmi_pdev_aux_count(flags);
    if (args.x0) {
        LOG(ERROR, "PDEV AUX count ABI failed, ret value is: %x\n", args.x0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    aux_count = args.x1;

    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR)
        return VAL_TEST_PREP_SEQ_FAILED;

    if (val_pcie_find_doe_capability(&num_bdf, &pdev_dev.bdf,
                                                     &pdev_dev.doe_cap_base))
        return VAL_TEST_PREP_SEQ_FAILED;

    status = val_pcie_get_rootport((uint32_t)pdev_dev.bdf, &rp_bdf);
    if (status != 0)
        return VAL_TEST_PREP_SEQ_FAILED;


    /* Populate params */
    pdev_params->flags = flags;
    pdev_params->pdev_id = pdev_dev.bdf;
    pdev_params->ecam_addr = PLATFORM_OVERRIDE_PCIE_ECAM_BASE_ADDR_0;
    pdev_params->cert_id = 0;
    pdev_params->ncoh_ide_sid = 0;
    pdev_params->hash_algo = RMI_HASH_SHA_256;
    pdev_params->num_aux = aux_count;
    pdev_params->root_id = (uint16_t)rp_bdf;
    pdev_params->segment_id = PCIE_EXTRACT_BDF_SEG(pdev_dev.bdf);
    pdev_params->rid_base = PLATFORM_PCIE_RID_BASE;
    pdev_params->rid_top = PLATFORM_PCIE_RID_TOP + 1;

    pdev_dev.root_id = (uint16_t)rp_bdf;

    if (type == PARAMS_VALID)
    {
        pdev_dev.pdev = c_args.pdev_ptr_valid;
        c_args.pdev_dev_valid = pdev_dev;
    }

    /* Create all aux granules */
    for (i = 0; i < pdev_params->num_aux; i++) {
        uint64_t pdev_aux = g_delegated_prep_sequence();
        if (pdev_aux == VAL_TEST_PREP_SEQ_FAILED)
            return VAL_TEST_PREP_SEQ_FAILED;

        pdev_params->aux[i] = pdev_aux;
    }

    switch (type)
    {
        case PARAMS_VALID:
            break;

        case INVALID_DEVICE_ID:
            pdev_params->pdev_id = rp_bdf;
            break;

        case USED_DEVICE_ID:
            pdev_params->pdev_id = pdev_new_prep_sequence();
            break;

        case INVALID_ROOT_PORT_ID:
            pdev_params->root_id = (uint16_t)pdev_params->pdev_id;
            break;

        case INVALID_NUM_AUX:
            pdev_params->num_aux = aux_count + 1;
            break;

        case AUX_GRAN_UNALINED:
             for (i = 0; i < pdev_params->num_aux; i++) {
                uint64_t pdev_aux = g_delegated_prep_sequence();
                if (pdev_aux == VAL_TEST_PREP_SEQ_FAILED)
                    return VAL_TEST_PREP_SEQ_FAILED;
                pdev_params->aux[i] = g_unaligned_prep_sequence(pdev_aux);
            }
            break;

        case PARAMS_AUX_ALIASED:
            pdev_params->aux[1] = pdev_params->aux[0];
            break;

        case PARAMS_AUX_UNDELEGATED:
            for (i = 0; i < pdev_params->num_aux; i++)
                pdev_params->aux[i] = g_undelegated_prep_sequence();
            break;

        case INVALID_NCOH_IDE_SID:
            pdev_params->ncoh_ide_sid = 32;
            break;

        default:
            return VAL_TEST_PREP_SEQ_FAILED;
    }

    return (uint64_t)pdev_params;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    uint64_t pdev, ret;

    /* Allocate and delegate PDEV */
    pdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!pdev)
    {
        LOG(ERROR, "Failed to allocate memory for pdev.\n");
        goto exit;
    } else {
        ret = val_host_rmi_granule_delegate(pdev);
        if (ret)
        {
            LOG(ERROR, "PDEV delegation failed, pdev=0x%x, ret=0x%x.\n", pdev, ret);
            goto exit;
        }
    }

    c_args.pdev_ptr_valid = pdev;

    c_args.params_ptr_valid = params_prep_sequence(PARAMS_VALID);
    if (c_args.params_ptr_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    return VAL_SUCCESS;

exit:
    return VAL_TEST_PREP_SEQ_FAILED;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case PDEV_UNALIGNED:
            args->pdev_ptr = g_unaligned_prep_sequence(c_args.pdev_ptr_valid);
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case PDEV_OUTSIDE_OF_PERMITTED_PA:
            args->pdev_ptr = g_outside_of_permitted_pa_prep_sequence();
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case PDEV_DEV_MEM_MMIO:
            args->pdev_ptr = g_dev_mem_prep_sequence();
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case PDEV_GRAN_STATE_UNDELEGATED:
            args->pdev_ptr = g_undelegated_prep_sequence();
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case PARAMS_UNALIGNED:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = g_unaligned_prep_sequence(c_args.params_ptr_valid);
            break;

        case PARAMS_PAS_REALM:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = g_delegated_prep_sequence();
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case PARAMS_PAS_SECURE:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = g_secure_prep_sequence();
            break;

        case DEVICE_ID_INVALID:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = params_prep_sequence(INVALID_DEVICE_ID);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case DEVICE_ID_NOT_USED:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = params_prep_sequence(USED_DEVICE_ID);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case ROOT_PORT_ID_INVALID:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = params_prep_sequence(INVALID_ROOT_PORT_ID);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case RID_RANGE_INVALID:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = params_prep_sequence(INVALID_RID_RANGE);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case OVERLAPPED_RID_RANGE:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = params_prep_sequence(RID_RANGE_OVERLAPPED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case BASE_ADDRESS_RANGE_UNALIGNED:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = params_prep_sequence(PARAM_BASE_ADDRESS_RANGE_UNALIGNED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case TOP_ADDRESS_RANGE_UNALIGNED:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = params_prep_sequence(PARAM_TOP_ADDRESS_RANGE_UNALIGNED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case ADDR_RANGES_OUTSIDE_PERMITTED_MEMORY:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = params_prep_sequence(ADDRESS_INVALID_RANGE);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;
        case ADDR_RANGES_OVERLAP_WITHIN_PDEV:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = params_prep_sequence(OVERLAPPED_ADDRESS_RANGE_WITHIN_PDEV);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case ADDR_RANGES_OVERLAP_OTHER_PDEV:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = params_prep_sequence(OVERLAPPED_ADDRESS_RANGE_ANOTHER_PDEV);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case NUM_AUX_INVALID:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = params_prep_sequence(INVALID_NUM_AUX);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case AUX_UNALIGNED:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = params_prep_sequence(AUX_GRAN_UNALINED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case AUX_ALIASED:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = params_prep_sequence(PARAMS_AUX_ALIASED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case AUX_STATE_UNDELEGATED:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = params_prep_sequence(PARAMS_AUX_UNDELEGATED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case NCOH_IDE_SID_OUTSIDE_BOUND:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = params_prep_sequence(INVALID_NCOH_IDE_SID);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_pdev_create_host(void)
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

        cmd_ret = val_host_rmi_pdev_create(args.pdev_ptr, args.params_ptr);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_device;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n");
    cmd_ret = val_host_rmi_pdev_create(c_args.pdev_ptr_valid, c_args.params_ptr_valid);
    if (cmd_ret.x0 != 0)
    {
        LOG(ERROR, "Command failed. %x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    /* Check PDEV state is PDEV_NEW */
    if (val_host_check_pdev_state(c_args.pdev_ptr_valid, RMI_PDEV_NEW))
    {
        LOG(ERROR, "PDEV state check failed.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_device;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    c_args.pdev_dev_valid.pdev = c_args.pdev_ptr_valid;
    if (c_args.pdev_ptr_valid &&
        val_host_pdev_teardown(&c_args.pdev_dev_valid, c_args.pdev_ptr_valid))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

exit:
    return;
}
