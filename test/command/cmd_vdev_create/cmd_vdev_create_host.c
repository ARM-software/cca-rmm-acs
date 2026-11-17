/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_rmi_sro.h"
#include "rmi_vdev_create_data.h"
#include "command_common_host.h"
#include "val_host_pcie.h"

static uint32_t num_bdf;

#define INVALID_VDEV_ID_PARAM    1
#define TEST_VDEV_RANGE_SLOT     3U
#define TEST_VDEV_POP_SLOT       2U

static struct argument_store {
    uint64_t rd_valid;
    uint64_t pdev_ptr_valid;
    uint64_t vdev_ptr_valid;
    uint64_t params_ptr_valid;
    val_host_vdev_ts vdev_dev_valid;
    val_host_pdev_ts pdev_dev_valid;
    val_host_realm_ts realm_valid;
    val_host_tracking_test_region_ts track_region;
} c_args;

static struct invalid_argument_store {
    val_host_vdev_ts vdev_1;
    val_host_realm_ts realm_da_disabled;
    uint64_t params[sizeof(test_data) / sizeof(test_data[0])];
    uint32_t params_count;
    uint64_t vdev_nonconventional;
} c_args_invalid;

static struct missing_l2_state {
    val_host_pdev_ts pdev;
    val_host_vdev_ts vdev;
    val_host_realm_ts realm;
    uint64_t vdev_ptr;
    uint64_t params_ptr;
    uint32_t pdev_ready;
    uint32_t vdev_delegated;
} missing_l2;

struct arguments {
    uint64_t rd;
    uint64_t pdev_ptr;
    uint64_t vdev_ptr;
    uint64_t params_ptr;
};

typedef enum {
    PARAMS_VALID = 0x0,
    PARAMS_ADDR_RANGE_INVALID,
    PARAMS_ADDR_RANGE_OUTSIDE_STREAM,
    PARAMS_ADDR_RANGE_OCCUPIED,
    PARAMS_TDI_ID_NOT_IN_RANGE
} prep_seq_type;

static uint64_t params_prep_sequence(prep_seq_type type,
                                     const val_host_pdev_ts *pdev_dev)
{
    val_host_rmi_vdev_params_ts *vdev_params;
    val_host_vdev_flags_ts vdev_flags;
    uint64_t flags_vdev, i;

    val_memset(&vdev_flags, 0, sizeof(vdev_flags));
    val_memcpy(&flags_vdev, &vdev_flags, sizeof(vdev_flags));

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
    vdev_params->tdi_id = pdev_dev->bdf;
    vdev_params->num_addr_range = pdev_dev->ncoh_num_addr_range;
    for (i = 0; i < vdev_params->num_addr_range; i++)
        vdev_params->addr_range[i] = pdev_dev->ncoh_addr_range[i];

    if ((vdev_params->num_addr_range == 0U) ||
        ((vdev_params->addr_range[0].top - vdev_params->addr_range[0].base) <
         ((TEST_VDEV_RANGE_SLOT + 1U) * PAGE_SIZE)))
    {
        val_host_mem_free(vdev_params);
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    vdev_params->num_addr_range = 1U;
    vdev_params->addr_range[0].base += TEST_VDEV_RANGE_SLOT * PAGE_SIZE;
    vdev_params->addr_range[0].top = vdev_params->addr_range[0].base + PAGE_SIZE;

    switch (type)
    {
        case PARAMS_VALID:
            break;

        case PARAMS_ADDR_RANGE_INVALID:
            vdev_params->num_addr_range = 2U;
            vdev_params->addr_range[1] = vdev_params->addr_range[0];
            break;

        case PARAMS_ADDR_RANGE_OUTSIDE_STREAM:
            vdev_params->addr_range[0].base = PLATFORM_MEMORY_POOL_BASE;
            vdev_params->addr_range[0].top =
                PLATFORM_MEMORY_POOL_BASE + PAGE_SIZE;
            for (i = 0; i < pdev_dev->ncoh_num_addr_range; i++)
            {
                if ((vdev_params->addr_range[0].base <
                     pdev_dev->ncoh_addr_range[i].top) &&
                    (vdev_params->addr_range[0].top >
                     pdev_dev->ncoh_addr_range[i].base))
                {
                    val_host_mem_free(vdev_params);
                    return VAL_TEST_PREP_SEQ_FAILED;
                }
            }
            break;

        case PARAMS_ADDR_RANGE_OCCUPIED:
            if (pdev_dev->rid_top <= pdev_dev->rid_base + 1U)
            {
                val_host_mem_free(vdev_params);
                return VAL_TEST_PREP_SEQ_FAILED;
            }
            vdev_params->tdi_id = pdev_dev->rid_base;
            vdev_params->addr_range[0].base =
                pdev_dev->ncoh_addr_range[0].base;
            vdev_params->addr_range[0].top =
                vdev_params->addr_range[0].base + PAGE_SIZE;
            break;

        case PARAMS_TDI_ID_NOT_IN_RANGE:
            if (pdev_dev->rid_top > pdev_dev->rid_base)
                vdev_params->tdi_id = pdev_dev->rid_top;
            else
                vdev_params->tdi_id = (PCIE_CREATE_BDF_PACKED(pdev_dev->bdf)) + 1U;
            break;

        default:
            return VAL_TEST_PREP_SEQ_FAILED;
    }

    return (uint64_t)vdev_params;
}

static uint64_t invalid_params_prep_sequence(prep_seq_type type,
                                             const val_host_pdev_ts *pdev_dev)
{
    uint64_t params_ptr;

    params_ptr = params_prep_sequence(type, pdev_dev);
    if (params_ptr == VAL_TEST_PREP_SEQ_FAILED)
        return params_ptr;

    if (c_args_invalid.params_count >=
        (sizeof(c_args_invalid.params) / sizeof(c_args_invalid.params[0])))
    {
        val_host_mem_free((void *)params_ptr);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    c_args_invalid.params[c_args_invalid.params_count++] = params_ptr;
    return params_ptr;
}

static uint64_t create_occupied_vdev(val_host_vdev_ts *vdev_obj,
                                     uint64_t vdev_id,
                                     uint64_t tdi_id,
                                     uint64_t range_slot)
{
    val_host_rmi_vdev_params_ts *params;
    val_host_dev_comm_data_ts *comm_data = NULL;
    uint64_t req_addr = 0U, resp_addr = 0U;
    uint64_t vdev;
    uint64_t ret;

    val_memset(vdev_obj, 0, sizeof(*vdev_obj));
    vdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (vdev == 0U)
        return VAL_TEST_PREP_SEQ_FAILED;

    ret = val_host_rmi_granule_range_delegate(vdev, vdev + PAGE_SIZE);
    if (ret)
        goto free_vdev;

    params = (val_host_rmi_vdev_params_ts *)params_prep_sequence(
                                            PARAMS_VALID,
                                            &c_args.pdev_dev_valid);
    if ((uint64_t)params == VAL_TEST_PREP_SEQ_FAILED)
        goto undelegate_vdev;

    params->vdev_id = vdev_id;
    params->tdi_id = tdi_id;
    params->addr_range[0].base =
        c_args.pdev_dev_valid.ncoh_addr_range[0].base + range_slot * PAGE_SIZE;
    params->addr_range[0].top = params->addr_range[0].base + PAGE_SIZE;

    comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    req_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    resp_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((comm_data == NULL) || (req_addr == 0U) || (resp_addr == 0U))
    {
        val_host_mem_free(params);
        goto free_comm_data;
    }

    val_memset(comm_data, 0, PAGE_SIZE);
    comm_data->enter.req_addr = req_addr;
    comm_data->enter.rsp_addr = resp_addr;

    ret = val_host_rmi_vdev_create(c_args.rd_valid, c_args.pdev_ptr_valid,
                                   vdev, (uint64_t)params);
    val_host_mem_free(params);
    if (ret)
        goto free_comm_data;

    vdev_obj->vdev = vdev;
    vdev_obj->pdev = c_args.pdev_ptr_valid;
    vdev_obj->realm_rd = c_args.rd_valid;
    vdev_obj->vdev_id = vdev_id;
    vdev_obj->tdi_id = tdi_id;
    vdev_obj->dev_comm_data = comm_data;

    if (val_host_dev_communicate(&c_args.realm_valid, &c_args.pdev_dev_valid,
                                 vdev_obj, RMI_VDEV_UNLOCKED))
    {
        LOG(ERROR, "Occupied VDEV initialization failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return VAL_SUCCESS;

free_comm_data:
    if (resp_addr != 0U)
        val_host_mem_free((void *)resp_addr);
    if (req_addr != 0U)
        val_host_mem_free((void *)req_addr);
    if (comm_data != NULL)
        val_host_mem_free(comm_data);
undelegate_vdev:
    (void)val_host_rmi_granule_range_undelegate(vdev, vdev + PAGE_SIZE);
free_vdev:
    val_host_mem_free((void *)vdev);
    return VAL_TEST_PREP_SEQ_FAILED;
}

static uint64_t occupied_vdev_prep_sequence(void)
{
    if (c_args_invalid.vdev_1.vdev != 0U)
        return VAL_SUCCESS;

    if (c_args.pdev_dev_valid.rid_top <= c_args.pdev_dev_valid.rid_base + 1U)
        return VAL_TEST_PREP_SEQ_FAILED;

    return create_occupied_vdev(&c_args_invalid.vdev_1,
                                INVALID_VDEV_ID_PARAM,
                                c_args.pdev_dev_valid.rid_base + 1U,
                                0U);
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

    c_args_invalid.realm_da_disabled = realm;
    return realm.rd;

}

static uint64_t vdev_pop_prep_sequence(void)
{
    val_smc_param_ts tracking;
    uint64_t base = c_args.pdev_dev_valid.ncoh_addr_range[0].base;
    uint64_t top = c_args.pdev_dev_valid.ncoh_addr_range[0].top;
    uint64_t vdev;
    uint64_t ret;

    if ((top <= base) || ((top - base) < ((TEST_VDEV_POP_SLOT + 1U) * PAGE_SIZE)))
        return VAL_TEST_PREP_SEQ_FAILED;

    vdev = base + TEST_VDEV_POP_SLOT * PAGE_SIZE;
    if ((vdev & (PAGE_SIZE - 1U)) != 0U)
        return VAL_TEST_PREP_SEQ_FAILED;

    tracking = val_host_rmi_granule_tracking_get(vdev, vdev + PAGE_SIZE);
    if ((tracking.x0 != RMI_SUCCESS) ||
        (tracking.x1 != RMI_MEM_CATEGORY_DEV_NCOH) ||
        (tracking.x2 != RMI_TRACKING_FINE) ||
        (tracking.x3 < vdev + PAGE_SIZE))
    {
        LOG(ERROR, "VDEV population-test memory is not fine-tracked NCOH device memory\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    ret = val_host_rmi_granule_range_delegate(vdev, vdev + PAGE_SIZE);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "VDEV population-test granule delegation failed: 0x%lx\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    c_args_invalid.vdev_nonconventional = vdev;
    return vdev;
}

static uint64_t vdev_track_prep_sequence(void)
{
    return val_host_tracking_coarse_prep_sequence(&c_args.track_region);
}

static uint32_t vdev_track_cleanup(void)
{
    return val_host_tracking_coarse_cleanup(&c_args.track_region);
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
    pdev_dev.rid_base = PCIE_CREATE_BDF_PACKED(pdev_dev.bdf);
    pdev_dev.rid_top = pdev_dev.rid_base + 3U;

    val_host_set_pdev_flags(&pdev_flags);
    val_memcpy(&pdev_dev.pdev_flags, &pdev_flags, sizeof(pdev_flags));

    pdev_dev.pdev = g_pdev_ready_prep_sequence(&pdev_dev);
    if (pdev_dev.pdev == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.pdev_ptr_valid = pdev_dev.pdev;
    c_args.pdev_dev_valid = pdev_dev;
    val_memset(&c_args.vdev_dev_valid, 0, sizeof(c_args.vdev_dev_valid));
    c_args.vdev_dev_valid.pdev_stream = val_host_mem_alloc(
        PAGE_SIZE, sizeof(*c_args.vdev_dev_valid.pdev_stream));
    if ((c_args.vdev_dev_valid.pdev_stream == NULL) ||
        val_host_pdev_stream_setup(&c_args.pdev_dev_valid,
                                   c_args.pdev_dev_valid.root_id,
                                   PLATFORM_OVERRIDE_SMMUV3_BASE,
                                   c_args.vdev_dev_valid.pdev_stream))
    {
        LOG(ERROR, "PDEV stream setup failed\n");
        goto exit;
    }

    val_memset(&realm, 0, sizeof(realm));
    val_memset(&realm_flags, 0, sizeof(realm_flags));
    /* Overwrite Realm Parameters */
    realm_flags.da = RMI_FEATURE_TRUE;

    val_host_realm_params(&realm);
    realm.max_num_vdevs = 2;
    val_memcpy(&realm.flags, &realm_flags, sizeof(realm.flags));

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm, 1))
    {
        LOG(ERROR, "Realm setup failed\n");
        goto exit;
    }
    c_args.rd_valid = realm.rd;
    c_args.realm_valid = realm;

    /* Create VDEV */
    /* Allocate and delegate VDEV */
    vdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!vdev)
    {
        LOG(ERROR, "Failed to allocate memory for vdev\n");
        goto exit;
    } else {
        ret = val_host_rmi_granule_range_delegate(vdev, vdev + PAGE_SIZE);
        if (ret)
        {
            LOG(ERROR, "vdev delegation failed, vdev=0x%x, ret=0x%x\n", vdev, ret);
            goto exit;
        }
    }

    c_args.vdev_ptr_valid = vdev;
    c_args.vdev_dev_valid.vdev = 0;
    c_args.vdev_dev_valid.pdev = pdev_dev.pdev;
    c_args.vdev_dev_valid.realm_rd = realm.rd;
    c_args.params_ptr_valid = params_prep_sequence(PARAMS_VALID,
                                                   &c_args.pdev_dev_valid);
    if (c_args.params_ptr_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    return VAL_SUCCESS;

exit:
    return VAL_TEST_PREP_SEQ_FAILED;
}

static uint64_t psmmu_st_l2_cleanup(void)
{
    val_smc_param_ts state;

    if (missing_l2.vdev.vdev != 0U)
    {
        state = val_host_rmi_vdev_get_state(missing_l2.vdev.vdev);
        if (state.x0 != RMI_SUCCESS)
            return VAL_ERROR;
        if ((state.x1 == RMI_VDEV_NEW) &&
            val_host_dev_communicate(&missing_l2.realm, &missing_l2.pdev,
                                      &missing_l2.vdev, RMI_VDEV_UNLOCKED))
            return VAL_ERROR;
    }

    if ((missing_l2.vdev.vdev != 0U ||
         missing_l2.vdev.pdev_stream != NULL) &&
        val_host_vdev_teardown(&missing_l2.realm, &missing_l2.pdev,
                               &missing_l2.vdev))
        return VAL_ERROR;

    if (missing_l2.vdev_delegated)
    {
        if (val_host_rmi_granule_range_undelegate(
                missing_l2.vdev_ptr, missing_l2.vdev_ptr + PAGE_SIZE))
            return VAL_ERROR;
        missing_l2.vdev_delegated = 0U;
    }

    if (missing_l2.vdev_ptr != 0U)
    {
        val_host_mem_free((void *)missing_l2.vdev_ptr);
        missing_l2.vdev_ptr = 0U;
    }

    if (missing_l2.pdev_ready &&
        val_host_pdev_teardown(&missing_l2.pdev, missing_l2.pdev.pdev))
        return VAL_ERROR;

    missing_l2.pdev_ready = 0U;

    if (missing_l2.realm.rd && val_host_realm_destroy(missing_l2.realm.rd))
        return VAL_ERROR;

    missing_l2.realm.rd = 0U;

    if (missing_l2.params_ptr != 0U)
    {
        val_host_mem_free((void *)missing_l2.params_ptr);
        missing_l2.params_ptr = 0U;
    }

    if (missing_l2.vdev.dev_comm_data != NULL)
    {
        val_host_mem_free((void *)missing_l2.vdev.dev_comm_data->enter.req_addr);
        val_host_mem_free((void *)missing_l2.vdev.dev_comm_data->enter.rsp_addr);
        val_host_mem_free(missing_l2.vdev.dev_comm_data);
        missing_l2.vdev.dev_comm_data = NULL;
    }

    return VAL_SUCCESS;
}

static uint64_t psmmu_st_l2_prep_sequence(struct arguments *args)
{
    val_host_realm_flags_ts realm_flags;
    val_host_pdev_flags_ts pdev_flags;
    uint64_t ret;
    uint32_t count, rp_bdf;
    bool found = false;

    val_memset(&missing_l2, 0, sizeof(missing_l2));
    count = val_pcie_get_num_bdf();
    if (count == VAL_ERROR)
        return VAL_TEST_PREP_SEQ_FAILED;

    /* Keep the first endpoint, its VDEV and its L2 table untouched. */
    while (count != 0U)
    {
        if (val_pcie_find_doe_capability(&count, &missing_l2.pdev.bdf,
                                         &missing_l2.pdev.doe_cap_base))
            break;
        if ((val_pcie_device_port_type(missing_l2.pdev.bdf) != EP) ||
            val_pcie_get_rootport(missing_l2.pdev.bdf, &rp_bdf) ||
            (rp_bdf == c_args.pdev_dev_valid.root_id))
            continue;

        val_host_get_addr_range(&missing_l2.pdev);
        if ((missing_l2.pdev.ncoh_num_addr_range == 0U) ||
            (missing_l2.pdev.ncoh_addr_range[0].top <=
             missing_l2.pdev.ncoh_addr_range[0].base) ||
            (missing_l2.pdev.ncoh_addr_range[0].top -
             missing_l2.pdev.ncoh_addr_range[0].base <
             (TEST_VDEV_RANGE_SLOT + 1U) * PAGE_SIZE))
            continue;

        found = true;
        break;
    }

    if (!found)
    {
        LOG(ERROR, "No second root-port endpoint with a VDEV range found\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    missing_l2.pdev.root_id = (uint16_t)rp_bdf;
    missing_l2.pdev.rid_base = PCIE_CREATE_BDF_PACKED(missing_l2.pdev.bdf);
    missing_l2.pdev.rid_top = missing_l2.pdev.rid_base + 1U;
    val_host_set_pdev_flags(&pdev_flags);
    missing_l2.pdev.pdev_flags = pdev_flags;
    missing_l2.pdev.pdev = g_pdev_ready_prep_sequence(&missing_l2.pdev);
    if (missing_l2.pdev.pdev == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    missing_l2.pdev_ready = 1U;

    missing_l2.vdev.pdev = missing_l2.pdev.pdev;
    missing_l2.vdev.pdev_stream = val_host_mem_alloc(
        PAGE_SIZE, sizeof(*missing_l2.vdev.pdev_stream));
    if ((missing_l2.vdev.pdev_stream == NULL) ||
        val_host_pdev_stream_setup(
            &missing_l2.pdev, missing_l2.pdev.root_id,
            PLATFORM_OVERRIDE_SMMUV3_BASE, missing_l2.vdev.pdev_stream))
        return VAL_TEST_PREP_SEQ_FAILED;

    ret = val_host_rmi_psmmu_st_l2_destroy(
        missing_l2.vdev.pdev_stream->psmmu,
        missing_l2.vdev.pdev_stream->endpoint_bdf);
    if (ret)
    {
        LOG(ERROR, "PSMMU L2 stream table destroy failed, ret=0x%x\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }
    missing_l2.vdev.pdev_stream->psmmu_l2_created = 0U;

    val_memset(&realm_flags, 0, sizeof(realm_flags));
    realm_flags.da = RMI_FEATURE_TRUE;
    val_host_realm_params(&missing_l2.realm);
    missing_l2.realm.vmid = 2U;
    missing_l2.realm.max_num_vdevs = 1U;
    val_memcpy(&missing_l2.realm.flags, &realm_flags,
               sizeof(missing_l2.realm.flags));
    if (val_host_realm_setup(&missing_l2.realm, 1))
        return VAL_TEST_PREP_SEQ_FAILED;

    missing_l2.vdev_ptr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (missing_l2.vdev_ptr == 0U)
        return VAL_TEST_PREP_SEQ_FAILED;
    if (val_host_rmi_granule_range_delegate(
            missing_l2.vdev_ptr, missing_l2.vdev_ptr + PAGE_SIZE))
        return VAL_TEST_PREP_SEQ_FAILED;

    missing_l2.vdev_delegated = 1U;

    missing_l2.params_ptr = params_prep_sequence(PARAMS_VALID,
                                                 &missing_l2.pdev);
    if (missing_l2.params_ptr == VAL_TEST_PREP_SEQ_FAILED)
    {
        missing_l2.params_ptr = 0U;
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    missing_l2.vdev.dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (missing_l2.vdev.dev_comm_data == NULL)
        return VAL_TEST_PREP_SEQ_FAILED;

    val_memset(missing_l2.vdev.dev_comm_data, 0, PAGE_SIZE);
    missing_l2.vdev.dev_comm_data->enter.req_addr =
        (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    missing_l2.vdev.dev_comm_data->enter.rsp_addr =
        (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((missing_l2.vdev.dev_comm_data->enter.req_addr == 0U) ||
        (missing_l2.vdev.dev_comm_data->enter.rsp_addr == 0U))
        return VAL_TEST_PREP_SEQ_FAILED;

    missing_l2.vdev.realm_rd = missing_l2.realm.rd;
    args->rd = missing_l2.realm.rd;
    args->pdev_ptr = missing_l2.pdev.pdev;
    args->vdev_ptr = missing_l2.vdev_ptr;
    args->params_ptr = missing_l2.params_ptr;
    return VAL_SUCCESS;
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


        case RD_STATE_UNDELEGATED:
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


        case PDEV_GRAN_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = g_undelegated_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case PDEV_CATEGORY_ROOT_PORT:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.vdev_dev_valid.pdev_stream->rp_pdev_obj.pdev;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = invalid_params_prep_sequence(
                PARAMS_VALID, &c_args.pdev_dev_valid);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            ((val_host_rmi_vdev_params_ts *)args->params_ptr)->tdi_id =
                PCIE_CREATE_BDF_PACKED(
                    c_args.vdev_dev_valid.pdev_stream->rp_pdev_obj.bdf);
            break;

        case ADDR_RANGE_INVALID:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = invalid_params_prep_sequence(
                                        PARAMS_ADDR_RANGE_INVALID,
                                        &c_args.pdev_dev_valid);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case ADDR_RANGE_OUTSIDE_STREAM:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = invalid_params_prep_sequence(
                PARAMS_ADDR_RANGE_OUTSIDE_STREAM, &c_args.pdev_dev_valid);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case ADDR_RANGE_OCCUPIED:
            if (occupied_vdev_prep_sequence())
                return VAL_ERROR;

            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = invalid_params_prep_sequence(
                PARAMS_ADDR_RANGE_OCCUPIED, &c_args.pdev_dev_valid);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case TDI_ID_NOT_IN_RANGE:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = invalid_params_prep_sequence(
                                        PARAMS_TDI_ID_NOT_IN_RANGE,
                                        &c_args.pdev_dev_valid);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case VDEV_ID_NOT_FREE:
            if (occupied_vdev_prep_sequence())
                return VAL_ERROR;

            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = invalid_params_prep_sequence(
                                        PARAMS_VALID,
                                        &c_args.pdev_dev_valid);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            ((val_host_rmi_vdev_params_ts *)args->params_ptr)->vdev_id =
                                        c_args_invalid.vdev_1.vdev_id;
            ((val_host_rmi_vdev_params_ts *)args->params_ptr)->tdi_id =
                                        c_args.pdev_dev_valid.rid_base;
            break;

        case TDI_ID_NOT_FREE:
            if (occupied_vdev_prep_sequence())
                return VAL_ERROR;

            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->params_ptr = invalid_params_prep_sequence(
                                        PARAMS_VALID,
                                        &c_args.pdev_dev_valid);
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            ((val_host_rmi_vdev_params_ts *)args->params_ptr)->tdi_id =
                                        c_args_invalid.vdev_1.tdi_id;
            break;

        case VDEV_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = g_unaligned_prep_sequence(c_args.vdev_ptr_valid);
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case VDEV_NONCONVENTIONAL:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = vdev_pop_prep_sequence();
            args->params_ptr = c_args.params_ptr_valid;
            if (args->vdev_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case VDEV_TRACK_COARSE:
            args->rd = c_args.rd_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = c_args.params_ptr_valid;
            args->vdev_ptr = vdev_track_prep_sequence();
            if (args->vdev_ptr == VAL_SKIP_CHECK)
                return VAL_SKIP_CHECK;
            if (args->vdev_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
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

        case PSMMU_ST_L2_MISSING:
            if (psmmu_st_l2_prep_sequence(args) == VAL_TEST_PREP_SEQ_FAILED)
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
    uint64_t ret;
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

        ret = intent_to_seq(&test_data[i], &args);
        if (ret == VAL_SKIP_CHECK)
        {
            LOG(TEST, "Skipping Check %d\n", (int)(i + 1));
            continue;
        }
        if (ret != VAL_SUCCESS) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto destroy_device;
        }

        cmd_ret = val_host_rmi_vdev_create_smc(args.rd, args.pdev_ptr,
                                      args.vdev_ptr, args.params_ptr);

        if (test_data[i].label == VDEV_TRACK_COARSE)
        {
            if ((RMI_STATUS(cmd_ret.x0) != RMI_ERROR_TRACKING) ||
                (RMI_TRACKING_LEVEL(cmd_ret.x0) != RMI_TRACKING_LEVEL_COARSE) ||
                (VAL_EXTRACT_BITS(cmd_ret.x0, 16, 55) !=
                 (args.vdev_ptr >> PAGE_BITS_4K)))
            {
                LOG(ERROR, "VDEV tracking check returned 0x%lx, expected "
                    "TRACKING at 0x%lx\n", cmd_ret.x0, args.vdev_ptr);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
                goto destroy_device;
            }

            if (vdev_track_cleanup() != VAL_SUCCESS)
            {
                LOG(ERROR, "VDEV tracking setup cleanup failed\n");
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
                goto destroy_device;
            }
            continue;
        }

        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_device;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n");
    ret = val_host_rmi_vdev_create(c_args.rd_valid, c_args.pdev_ptr_valid,
                                   c_args.vdev_ptr_valid, c_args.params_ptr_valid);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "Command failed. %x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_device;
    }

    c_args.vdev_dev_valid.vdev = c_args.vdev_ptr_valid;

    cmd_ret = val_host_rmi_vdev_get_state(c_args.vdev_ptr_valid);
    if (cmd_ret.x0)
    {
        LOG(ERROR, "VDEV_GET_STATE failed, ret %lx.\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_device;
    }

    /* Check vdev state is VDEV_NEW */
    if (VAL_EXTRACT_BITS(cmd_ret.x1, 0, 7) != RMI_VDEV_NEW)
    {
        LOG(ERROR, "VDEV state should be VDEV_NEW\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto destroy_device;
    }

    c_args.vdev_dev_valid.dev_comm_data =
        val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (c_args.vdev_dev_valid.dev_comm_data == NULL)
    {
        LOG(ERROR, "VDEV communication data allocation failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_device;
    }

    val_memset(c_args.vdev_dev_valid.dev_comm_data, 0, PAGE_SIZE);
    c_args.vdev_dev_valid.dev_comm_data->enter.req_addr =
        (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    c_args.vdev_dev_valid.dev_comm_data->enter.rsp_addr =
        (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);

    if ((c_args.vdev_dev_valid.dev_comm_data->enter.req_addr == 0U) ||
        (c_args.vdev_dev_valid.dev_comm_data->enter.rsp_addr == 0U) ||
        val_host_dev_communicate(&c_args.realm_valid, &c_args.pdev_dev_valid,
                                 &c_args.vdev_dev_valid, RMI_VDEV_UNLOCKED))
    {
        LOG(ERROR, "Positive VDEV initialization failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_device;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    if (vdev_track_cleanup() != VAL_SUCCESS)
    {
        LOG(ERROR, "VDEV tracking setup cleanup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    if (psmmu_st_l2_cleanup())
    {
        LOG(ERROR, "Independent psmmu_st_l2 setup cleanup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    while (c_args_invalid.params_count != 0U)
        val_host_mem_free((void *)c_args_invalid.params[
                                    --c_args_invalid.params_count]);

    if (c_args_invalid.vdev_nonconventional != 0U)
    {
        if (val_host_rmi_granule_range_undelegate(
                c_args_invalid.vdev_nonconventional,
                c_args_invalid.vdev_nonconventional + PAGE_SIZE))
        {
            LOG(ERROR, "VDEV population-test granule cleanup failed\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(14)));
            goto exit;
        }
        c_args_invalid.vdev_nonconventional = 0U;
    }

    if (val_host_vdev_teardown(&c_args.realm_valid, &c_args.pdev_dev_valid,
                               &c_args_invalid.vdev_1))
    {
        LOG(ERROR, "VDEV 1 cleanup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(15)));
        goto exit;
    }

    if ((c_args.vdev_dev_valid.vdev || c_args.vdev_dev_valid.pdev_stream) &&
        val_host_vdev_teardown(&c_args.realm_valid, &c_args.pdev_dev_valid,
                               &c_args.vdev_dev_valid))
    {
        LOG(ERROR, "VDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(17)));
        goto exit;
    }

    if (c_args.pdev_ptr_valid &&
        val_host_pdev_teardown(&c_args.pdev_dev_valid, c_args.pdev_ptr_valid))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(18)));
        goto exit;
    }

    if (c_args_invalid.realm_da_disabled.rd &&
        val_host_realm_destroy(c_args_invalid.realm_da_disabled.rd))
    {
        LOG(ERROR, "DA-disabled Realm teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(19)));
    }

exit:
    return;
}
