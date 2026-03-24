/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_vdev_create_data.h"
#include "command_common_host.h"
#include "val_host_pcie.h"

static uint32_t num_bdf;

#define INVALID_VDEV_ID_PARAM    1
#define INVALID_TDI_ID_PARAM     1

static struct argument_store {
    uint64_t rd_valid;
    uint64_t pdev_ptr_valid;
    uint64_t vdev_ptr_valid;
    uint64_t params_ptr_valid;
    val_host_vdev_ts vdev_dev_valid;
    val_host_pdev_ts pdev_dev_valid;
    val_host_realm_ts realm_valid;
} c_args;

static struct invalid_argument_store {
    uint64_t pdev_new_ptr;
    val_host_pdev_ts pdev_dev_pdev_new_valid;
    uint64_t pdev_ready_invalid_category_ptr;
    val_host_pdev_ts pdev_dev_pdev_ready_valid;
} c_args_invalid;

struct arguments {
    uint64_t rd;
    uint64_t pdev_ptr;
    uint64_t vdev_ptr;
    uint64_t params_ptr;
};

typedef enum {
    PARAMS_VALID = 0x0,
    PARAMS_NUM_AUX_INVALID,
    PARAMS_AUX_UNALIGNED,
    PARAMS_AUX_ALIASED,
    PARAMS_AUX_STATE_UNDELEGATED,
    PARAMS_VDEV_ID_USED,
    PARAMS_TDI_ID_USED,
    PARAMS_TDI_ID_NOT_IN_RANGE
} prep_seq_type;

static uint64_t params_prep_sequence(prep_seq_type type)
{
    val_host_vdev_params_ts *vdev_params;
    val_host_pdev_flags_ts pdev_flags;
    val_host_vdev_flags_ts vdev_flags;
    uint64_t flags_pdev, flags_vdev, aux_count, i;
    val_smc_param_ts args;

    val_memset(&pdev_flags, 0, sizeof(pdev_flags));
    val_memset(&vdev_flags, 0, sizeof(vdev_flags));

    pdev_flags.spdm = RMI_SPDM_TRUE;
    pdev_flags.ncoh_ide = RMI_IDE_TRUE;
    pdev_flags.category = RMI_PDEV_SMEM;

    val_memcpy(&flags_pdev, &pdev_flags, sizeof(pdev_flags));
    val_memcpy(&flags_vdev, &vdev_flags, sizeof(vdev_flags));

    /* Get aux granules count */
    args = val_host_rmi_vdev_aux_count(flags_pdev, flags_vdev);
    if (args.x0) {
        LOG(ERROR, "VDEV AUX count ABI failed, ret value is: %x\n", args.x0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    aux_count = args.x1;

    /* Allocate memory for params */
    vdev_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (vdev_params == NULL)
    {
        LOG(ERROR, "Failed to allocate memory for vdev_params.\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    val_memset(vdev_params, 0, PAGE_SIZE_4K);

    /* Populate params */
    vdev_params->flags = flags_vdev;
    vdev_params->vdev_id = 0;
    vdev_params->tdi_id = c_args.pdev_dev_valid.bdf;
    vdev_params->num_aux = aux_count;

    /* Create all aux granules */
    for (i = 0; i < vdev_params->num_aux; i++) {
        uint64_t vdev_aux = g_delegated_prep_sequence();
        if (vdev_aux == VAL_TEST_PREP_SEQ_FAILED)
            return VAL_TEST_PREP_SEQ_FAILED;

        vdev_params->aux[i] = vdev_aux;
    }

    switch (type)
    {
        case PARAMS_VALID:
            break;

        case PARAMS_NUM_AUX_INVALID:
            vdev_params->num_aux = vdev_params->num_aux + 1;
            break;

        case PARAMS_AUX_UNALIGNED:
             for (i = 0; i < vdev_params->num_aux; i++) {
                uint64_t pdev_aux = g_delegated_prep_sequence();
                if (pdev_aux == VAL_TEST_PREP_SEQ_FAILED)
                    return VAL_TEST_PREP_SEQ_FAILED;
                vdev_params->aux[i] = g_unaligned_prep_sequence(pdev_aux);
            }
            break;

        case PARAMS_AUX_ALIASED:
            vdev_params->aux[1] = vdev_params->aux[0];
            break;

        case PARAMS_AUX_STATE_UNDELEGATED:
            for (i = 0; i < vdev_params->num_aux; i++)
                vdev_params->aux[i] = g_undelegated_prep_sequence();
            break;

        case PARAMS_VDEV_ID_USED:
            vdev_params->vdev_id = INVALID_VDEV_ID_PARAM;
            break;

        case PARAMS_TDI_ID_USED:
            vdev_params->vdev_id = INVALID_TDI_ID_PARAM;
            break;

        /*TODO */
        case PARAMS_TDI_ID_NOT_IN_RANGE:
            vdev_params->vdev_id = INVALID_TDI_ID_PARAM;
            break;

        default:
            return VAL_TEST_PREP_SEQ_FAILED;
    }

    return (uint64_t)vdev_params;
}

static uint64_t pdev_new_prep_sequence(void)
{
    val_host_pdev_ts pdev_dev;
    uint32_t rp_bdf, status;
    val_host_pdev_flags_ts pdev_flags;

    val_memset(&pdev_dev, 0, sizeof(pdev_dev));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

    if (val_pcie_find_doe_capability(&num_bdf, &pdev_dev.bdf,
                                                     &pdev_dev.doe_cap_base))
        return VAL_TEST_PREP_SEQ_FAILED;

    status = val_pcie_get_rootport((uint32_t)pdev_dev.bdf, &rp_bdf);
    if (status != 0)
        return VAL_TEST_PREP_SEQ_FAILED;

    pdev_dev.root_id = (uint16_t)rp_bdf;

    val_host_set_pdev_flags(&pdev_flags);
    val_memcpy(&pdev_dev.pdev_flags, &pdev_flags, sizeof(pdev_flags));

    pdev_dev.pdev = g_pdev_new_prep_sequence(&pdev_dev);
    if (pdev_dev.pdev == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args_invalid.pdev_new_ptr = pdev_dev.pdev;
    c_args_invalid.pdev_dev_pdev_new_valid = pdev_dev;

    return pdev_dev.pdev;
}

static uint64_t pdev_ready_category_cmem_cxl_prep_sequence(void)
{
    val_host_pdev_ts pdev_dev;
    uint32_t rp_bdf, status;
    val_host_pdev_flags_ts pdev_flags;

    val_memset(&pdev_dev, 0, sizeof(pdev_dev));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

    if (val_pcie_find_doe_capability(&num_bdf, &pdev_dev.bdf,
                                                     &pdev_dev.doe_cap_base))
        return VAL_TEST_PREP_SEQ_FAILED;

    status = val_pcie_get_rootport((uint32_t)pdev_dev.bdf, &rp_bdf);
    if (status != 0)
        return VAL_TEST_PREP_SEQ_FAILED;

    pdev_dev.root_id = (uint16_t)rp_bdf;

    val_host_set_pdev_flags(&pdev_flags);
    pdev_flags.category = RMI_PDEV_CMEM_CXL;
    val_memcpy(&pdev_dev.pdev_flags, &pdev_flags, sizeof(pdev_flags));

    pdev_dev.pdev = g_pdev_ready_prep_sequence(&pdev_dev);
    if (pdev_dev.pdev == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args_invalid.pdev_ready_invalid_category_ptr = pdev_dev.pdev;
    c_args_invalid.pdev_dev_pdev_ready_valid = pdev_dev;

    return pdev_dev.pdev;
}

static uint64_t da_disable_realm_prep_sequence(void)
{
    val_host_realm_ts realm;
    val_host_realm_flags_ts realm_flags;

    val_memset(&realm, 0, sizeof(realm));
    val_memset(&realm_flags, 0, sizeof(realm_flags));

    val_host_realm_params(&realm);
    realm.vmid = 1;

    val_memcpy(&realm.flags, &realm_flags, sizeof(realm.flags));

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm, 1))
    {
        LOG(ERROR, "\tRealm setup failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm.rd;

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
            LOG(ERROR, "vdev delegation failed, vdev=0x%x, ret=0x%x\n", vdev, ret);
            goto exit;
        }
    }

    args = val_host_rmi_vdev_aux_count(flags_pdev, flags_vdev);
    if (args.x0) {
        LOG(ERROR, "VDEV AUX count ABI failed, ret value is: %x\n", args.x0);
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
    vdev_params->vdev_id = INVALID_VDEV_ID_PARAM;
    vdev_params->tdi_id = INVALID_TDI_ID_PARAM;
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
                LOG(ERROR, "vdev delegation failed, vdev=0x%x, ret=0x%x\n", vdev_params->aux[j],
                                                                               ret);
                goto exit;
            }
        }
    }

    /* Create vdev */
    args = val_host_rmi_vdev_create(c_args.rd_valid, c_args.pdev_ptr_valid, vdev,
                                                          (uint64_t)vdev_params);
    if (args.x0)
    {
        LOG(ERROR, "VDEV create failed.\n");
        goto exit;
    }

    args = val_host_rmi_vdev_get_state(vdev);
    if (args.x0)
    {

        LOG(ERROR, "VDEV get state failed ret %x\n", args.x0);
        goto exit;
    }

    /* Check VDEV state is RMI_VDEV_NEW */
    if (args.x1 != RMI_VDEV_NEW)
    {
        LOG(ERROR, "VDEV state should be RMI_VDEV_NEW, ret %lx\n", args.x1);
        goto exit;
    }

    return VAL_SUCCESS;

exit:
    return VAL_TEST_PREP_SEQ_FAILED;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    val_host_pdev_ts pdev_dev;
    val_host_realm_ts realm;
    val_host_realm_flags_ts realm_flags;
    uint64_t vdev, ret;
    uint32_t rp_bdf, status;
    val_host_pdev_flags_ts pdev_flags;

    val_memset(&pdev_dev, 0, sizeof(pdev_dev));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));
    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR)
        return VAL_TEST_PREP_SEQ_FAILED;

    if (val_pcie_find_doe_capability(&num_bdf, &pdev_dev.bdf,
                                     &pdev_dev.doe_cap_base))
        return VAL_TEST_PREP_SEQ_FAILED;

    status = val_pcie_get_rootport((uint32_t)pdev_dev.bdf, &rp_bdf);
    if (status != 0)
        return VAL_TEST_PREP_SEQ_FAILED;

    pdev_dev.root_id = (uint16_t)rp_bdf;

    val_host_set_pdev_flags(&pdev_flags);
    val_memcpy(&pdev_dev.pdev_flags, &pdev_flags, sizeof(pdev_flags));

    pdev_dev.pdev = g_pdev_ready_prep_sequence(&pdev_dev);
    if (pdev_dev.pdev == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    val_memset(&realm, 0, sizeof(realm));
    val_memset(&realm_flags, 0, sizeof(realm_flags));
    /* Overwrite Realm Parameters */
    realm_flags.da = RMI_FEATURE_TRUE;

    val_host_realm_params(&realm);
    val_memcpy(&realm.flags, &realm_flags, sizeof(realm.flags));

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm, 1))
    {
        LOG(ERROR, "Realm setup failed\n");
        goto exit;
    }

    /* Create VDEV */
    /* Allocate and delegate VDEV */
    vdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!vdev)
    {
        LOG(ERROR, "Failed to allocate memory for vdev\n");
        goto exit;
    } else {
        ret = val_host_rmi_granule_delegate(vdev);
        if (ret)
        {
            LOG(ERROR, "vdev delegation failed, vdev=0x%x, ret=0x%x\n", vdev, ret);
            goto exit;
        }
    }

    c_args.rd_valid = realm.rd;
    c_args.pdev_ptr_valid = pdev_dev.pdev;
    c_args.vdev_ptr_valid = vdev;
    val_memset(&c_args.vdev_dev_valid, 0, sizeof(c_args.vdev_dev_valid));
    c_args.vdev_dev_valid.vdev = vdev;
    c_args.vdev_dev_valid.pdev = pdev_dev.pdev;
    c_args.vdev_dev_valid.realm_rd = realm.rd;
    c_args.pdev_dev_valid = pdev_dev;
    c_args.realm_valid = realm;
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
        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case RD_OUTSIDE_OF_PERMITTED_PA:
            args->rd = g_outside_of_permitted_pa_prep_sequence();
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case RD_DEV_MEM_MMIO:
            args->rd = g_dev_mem_prep_sequence();
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case RD_GRAN_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case PDEV_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = g_unaligned_prep_sequence(c_args.pdev_ptr_valid);
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case PDEV_OUTSIDE_OF_PERMITTED_PA:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = g_outside_of_permitted_pa_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case PDEV_DEV_MEM_MMIO:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = g_dev_mem_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case PDEV_GRAN_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = g_undelegated_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case PDEV_STATE_NEW:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = pdev_new_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case PDEV_CATEGORY_CMEM_CXL:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = pdev_ready_category_cmem_cxl_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case VDEV_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = g_unaligned_prep_sequence(c_args.vdev_ptr_valid);
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case VDEV_OUTSIDE_OF_PERMITTED_PA:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = g_outside_of_permitted_pa_prep_sequence();
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case VDEV_DEV_MEM_MMIO:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = g_dev_mem_prep_sequence();
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case VDEV_GRAN_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = g_undelegated_prep_sequence();
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case PARAMS_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = g_unaligned_prep_sequence(c_args.params_ptr_valid);
            break;

        case PARAMS_PAS_REALM:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = g_delegated_prep_sequence();
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case PARAMS_PAS_SECURE:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = g_secure_prep_sequence();
            break;

        case DA_FEATURE_FALSE:
            args->rd = da_disable_realm_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case NUM_AUX_INVALID:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = params_prep_sequence(PARAMS_NUM_AUX_INVALID);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case AUX_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = params_prep_sequence(PARAMS_AUX_UNALIGNED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case AUX_ALIASED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = params_prep_sequence(PARAMS_AUX_ALIASED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case AUX_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = params_prep_sequence(PARAMS_AUX_STATE_UNDELEGATED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case VDEV_ID_USED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            if (invalid_vdev_id_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->params_ptr = params_prep_sequence(PARAMS_VDEV_ID_USED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case TDI_ID_USED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = params_prep_sequence(PARAMS_TDI_ID_USED);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case TDI_ID_NOT_IN_RANGE:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = params_prep_sequence(PARAMS_TDI_ID_NOT_IN_RANGE);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_vdev_create_host(void)
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

        cmd_ret = val_host_rmi_vdev_create(args.rd, args.pdev_ptr, args.vdev_ptr, args.params_ptr);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_device;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n");
    cmd_ret = val_host_rmi_vdev_create(c_args.rd_valid, c_args.pdev_ptr_valid,
                              c_args.vdev_ptr_valid, c_args.params_ptr_valid);
    if (cmd_ret.x0 != 0)
    {
        LOG(ERROR, "Command failed. %x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    cmd_ret = val_host_rmi_vdev_get_state(c_args.vdev_ptr_valid);
    if (cmd_ret.x0)
    {
        LOG(ERROR, "VDEV_GET_STATE failed, ret %lx.\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_device;
    }

    /* Check vdev state is VDEV_NEw */
    if (cmd_ret.x1 != RMI_VDEV_NEW)
    {
        LOG(ERROR, "VDEV state should be VDEV_NEW\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_device;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    if (c_args.vdev_ptr_valid &&
        val_host_vdev_teardown(&c_args.realm_valid, &c_args.pdev_dev_valid,
                               &c_args.vdev_dev_valid))
    {
        LOG(ERROR, "VDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    if (c_args.pdev_ptr_valid &&
        val_host_pdev_teardown(&c_args.pdev_dev_valid, c_args.pdev_ptr_valid))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    if (c_args_invalid.pdev_new_ptr &&
        val_host_pdev_teardown(&c_args_invalid.pdev_dev_pdev_new_valid,
                                          c_args_invalid.pdev_new_ptr))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

    if (c_args_invalid.pdev_ready_invalid_category_ptr &&
        val_host_pdev_teardown(&c_args_invalid.pdev_dev_pdev_ready_valid,
                                          c_args_invalid.pdev_ready_invalid_category_ptr))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto exit;
    }

exit:
    return;
}
