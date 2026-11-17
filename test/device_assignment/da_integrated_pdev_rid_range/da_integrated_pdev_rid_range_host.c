/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "platform_override_fvp.h"
#include "val_host_da.h"
#include "val_host_helpers.h"
#include "val_host_pcie.h"
#include "val_host_rmi.h"
#include "val_host_rmi_sro.h"

#define DA_RID_RANGE_VALID         0U
#define DA_RID_RANGE_BASE_LOW      1U
#define DA_RID_RANGE_TOP_HIGH      2U

static uint64_t da_find_integrated_doe_endpoint(uint32_t *bdf, uint32_t *doe_cap_base)
{
    pcie_device_bdf_table *bdf_tbl_ptr;
    uint32_t cap_base;
    uint32_t pcie_bdf;
    uint32_t dp_type;
    uint32_t i;

    bdf_tbl_ptr = val_pcie_bdf_table_ptr();
    if ((bdf_tbl_ptr == NULL) || (bdf_tbl_ptr->num_entries == 0U))
    {
        LOG(ERROR, "No PCIe BDFs discovered\n");
        return VAL_ERROR;
    }

    for (i = 0U; i < bdf_tbl_ptr->num_entries; i++)
    {
        pcie_bdf = bdf_tbl_ptr->device[i].bdf;
        if (PCIE_EXTRACT_BDF_FUNC(pcie_bdf) != 0U)
            continue;

        dp_type = val_pcie_device_port_type(pcie_bdf);
        if ((dp_type != RCiEP) && (dp_type != iEP_EP) &&
            (val_pcie_is_onchip_peripheral(pcie_bdf) == 0U))
            continue;

        if (val_pcie_find_capability(pcie_bdf, PCIE_ECAP, DOE_CAP_ID,
                                     &cap_base) != PCIE_SUCCESS)
            continue;

        *bdf = pcie_bdf;
        *doe_cap_base = cap_base;
        LOG(TEST, "Integrated PDEV BDF=0x%x DOE cap=0x%x type=0x%x\n",
            pcie_bdf, cap_base, dp_type);
        return VAL_SUCCESS;
    }

    LOG(TEST, "Skipping: No integrated DOE-capable PDEV found\n");
    return VAL_SKIP_CHECK;
}

static void da_populate_integrated_pdev_params(val_host_rmi_pdev_params_ts *pdev_params,
                                               uint32_t bdf, uint32_t range_type)
{
    val_host_pdev_flags_ts pdev_flags;
    uint64_t flags = 0UL;

    val_memset(&pdev_flags, 0, sizeof(pdev_flags));
    pdev_flags.spdm = RMI_SPDM_TRUE;
    pdev_flags.category = RMI_PDEV_ENDPOINT_ACCEL_ON_CHIP;
    val_memcpy(&flags, &pdev_flags, sizeof(pdev_flags));

    val_memset(pdev_params, 0, PAGE_SIZE);
    pdev_params->flags = flags;
    pdev_params->hb_base = val_pcie_get_ecam_base(bdf);
    pdev_params->pdev_id = PCIE_CREATE_BDF_PACKED(bdf);
    pdev_params->routing_id = PCIE_EXTRACT_BDF_SEG(bdf);
    pdev_params->id_index = 0UL;
    pdev_params->rid_base = PLATFORM_PCIE_RID_BASE;
    pdev_params->rid_top = PLATFORM_PCIE_RID_TOP + 1U;
    pdev_params->hash_algo = RMI_HASH_SHA_512;

    if (range_type == DA_RID_RANGE_BASE_LOW)
        pdev_params->rid_base = PLATFORM_PCIE_RID_BASE - 1U;
    else if (range_type == DA_RID_RANGE_TOP_HIGH)
        pdev_params->rid_top = PLATFORM_PCIE_RID_TOP + 2U;
}

static uint64_t da_pdev_create_with_rid_range(uint32_t bdf, uint32_t range_type,
                                              uint64_t *pdev_out)
{
    val_host_rmi_pdev_params_ts *pdev_params;
    uint64_t pdev;
    uint64_t ret;

    *pdev_out = 0UL;

    pdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    pdev_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((pdev == 0UL) || (pdev_params == NULL))
    {
        LOG(ERROR, "Failed to allocate PDEV create resources\n");
        return VAL_ERROR;
    }

    ret = val_host_rmi_granule_range_delegate(pdev, pdev + PAGE_SIZE);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "Failed to delegate PDEV granule, ret=0x%lx\n", ret);
        return VAL_ERROR;
    }

    da_populate_integrated_pdev_params(pdev_params, bdf, range_type);

    LOG(TEST, "PDEV RID range base=0x%x top=0x%x platform=[0x%x,0x%x]\n",
        pdev_params->rid_base, pdev_params->rid_top,
        PLATFORM_PCIE_RID_BASE, PLATFORM_PCIE_RID_TOP + 1U);

    if (range_type == DA_RID_RANGE_VALID)
        ret = val_host_rmi_pdev_create(pdev, (uint64_t)pdev_params);
    else
        ret = RMI_STATUS(val_host_rmi_pdev_create_smc(pdev, (uint64_t)pdev_params).x0);

    if ((range_type != DA_RID_RANGE_VALID) ||
        ((ret != RMI_SUCCESS) && (ret != RMI_INCOMPLETE)))
    {
        uint64_t undelegate_ret;

        undelegate_ret = val_host_rmi_granule_range_undelegate(pdev, pdev + PAGE_SIZE);
        if (undelegate_ret != RMI_SUCCESS)
            LOG(ERROR, "PDEV granule undelegation failed, ret=0x%lx\n", undelegate_ret);
    }

    if (ret == RMI_SUCCESS)
        *pdev_out = pdev;

    return ret;
}

void da_integrated_pdev_rid_range_host(void)
{
    uint64_t pdev = 0UL;
    uint64_t ret;
    uint32_t bdf = 0U;
    uint32_t doe_cap_base = 0U;
    uint32_t packed_rid;

    if (!val_host_rmm_supports_da())
    {
        LOG(TEST, "Skipping: Device Assignment not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    ret = da_find_integrated_doe_endpoint(&bdf, &doe_cap_base);
    if (ret == VAL_SKIP_CHECK)
    {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    if (ret != VAL_SUCCESS)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    packed_rid = PCIE_CREATE_BDF_PACKED(bdf);
    if ((packed_rid < PLATFORM_PCIE_RID_BASE) ||
        (packed_rid > PLATFORM_PCIE_RID_TOP))
    {
        LOG(ERROR, "Integrated PDEV RID 0x%x outside platform range [0x%x,0x%x]\n",
            packed_rid, PLATFORM_PCIE_RID_BASE, PLATFORM_PCIE_RID_TOP);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return;
    }

    ret = da_pdev_create_with_rid_range(bdf, DA_RID_RANGE_VALID, &pdev);
    if (ret == RMI_ERROR_NOT_SUPPORTED)
    {
        LOG(TEST, "Skipping: Integrated PDEV creation is not supported by RMM\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "Valid integrated PDEV RID range was rejected, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        return;
    }

    ret = val_host_rmi_pdev_destroy(pdev);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_PDEV_DESTROY failed, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        return;
    }

    ret = da_pdev_create_with_rid_range(bdf, DA_RID_RANGE_BASE_LOW, &pdev);
    if (ret != RMI_ERROR_INPUT)
    {
        LOG(ERROR, "RID base below platform range was not rejected, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        return;
    }

    ret = da_pdev_create_with_rid_range(bdf, DA_RID_RANGE_TOP_HIGH, &pdev);
    if (ret != RMI_ERROR_INPUT)
    {
        LOG(ERROR, "RID top above platform range was not rejected, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        return;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));
}
