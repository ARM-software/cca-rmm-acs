/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_da.h"
#include "val_host_pcie.h"
#include "val_host_helpers.h"
#include "val_host_rmi.h"

static uint64_t da_find_doe_endpoint(uint32_t *bdf, uint32_t *doe_cap_base)
{
    uint32_t num_bdf;

    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR)
    {
        LOG(ERROR, "Failed to get PCIe BDF count\n");
        return VAL_ERROR;
    }

    if (val_pcie_find_doe_capability(&num_bdf, bdf, doe_cap_base) != VAL_SUCCESS)
    {
        LOG(TEST, "Skipping: No PCIe DOE capability supported device found\n");
        return VAL_SKIP_CHECK;
    }

    return VAL_SUCCESS;
}

static uint64_t da_get_rmm_max_vdevs_order(uint64_t *max_vdevs_order)
{
    uint64_t featreg2;
    uint64_t ret;

    ret = val_host_rmi_features(RMI_FEATURE_REGISTER_2_INDEX, &featreg2);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_FEATURES failed, ret=0x%lx\n", ret);
        return ret;
    }

    *max_vdevs_order = VAL_EXTRACT_BITS(featreg2,
                        RMI_FEATURE_REGISTER_2_MAX_VDEVS_ORDER_START,
                        RMI_FEATURE_REGISTER_2_MAX_VDEVS_ORDER_END);
    return RMI_SUCCESS;
}

void da_pdev_max_num_vdevs_host(void)
{
    val_host_rmi_pdev_params_ts *pdev_params;
    val_host_pdev_flags_ts pdev_flags;
    val_smc_param_ts args;
    uint64_t pdev = 0UL;
    uint64_t flags = 0UL;
    uint64_t max_vdevs_order;
    uint32_t invalid_max_num_vdevs;
    uint64_t ret;
    uint32_t bdf = 0U;
    uint32_t doe_cap_base = 0U;

    if (!val_host_rmm_supports_da())
    {
        LOG(TEST, "Skipping: Device Assignment not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    ret = da_find_doe_endpoint(&bdf, &doe_cap_base);
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

    ret = da_get_rmm_max_vdevs_order(&max_vdevs_order);
    if (ret != RMI_SUCCESS)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return;
    }

    invalid_max_num_vdevs = 1U << (uint32_t)max_vdevs_order;

    pdev = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    pdev_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((pdev == 0UL) || (pdev_params == NULL))
    {
        LOG(ERROR, "Failed to allocate PDEV test granules\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        return;
    }

    ret = val_host_rmi_granule_range_delegate(pdev, pdev + PAGE_SIZE);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "Failed to delegate PDEV granule, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        return;
    }

    val_memset(pdev_params, 0, PAGE_SIZE);
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));
    pdev_flags.spdm = RMI_SPDM_TRUE;
    pdev_flags.category = RMI_PDEV_ENDPOINT_ACCEL_OFF_CHIP;
    val_memcpy(&flags, &pdev_flags, sizeof(pdev_flags));

    pdev_params->flags = flags;
    pdev_params->hb_base = val_pcie_get_ecam_base(bdf);
    pdev_params->pdev_id = PCIE_CREATE_BDF_PACKED(bdf);
    pdev_params->routing_id = PCIE_EXTRACT_BDF_SEG(bdf);
    pdev_params->id_index = 0UL;
    pdev_params->rid_base = PCIE_CREATE_BDF_PACKED(bdf);
    pdev_params->rid_top = pdev_params->rid_base + invalid_max_num_vdevs;
    pdev_params->hash_algo = RMI_HASH_SHA_512;

    LOG(TEST, "Device BDF=0x%x DOE cap=0x%x RMM max_vdevs_order=%lu invalid max_num_vdevs=%u\n",
        bdf, doe_cap_base, max_vdevs_order, invalid_max_num_vdevs);

    args = val_host_rmi_pdev_create_smc(pdev, (uint64_t)pdev_params);
    if (RMI_STATUS(args.x0) != RMI_ERROR_INPUT)
    {
        LOG(ERROR, "RMI_PDEV_CREATE accepted invalid max_num_vdevs, ret=0x%lx\n",
            args.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto cleanup;
    }

    LOG(TEST, "RMI_PDEV_CREATE rejected max_num_vdevs=%u with RMI_ERROR_INPUT\n",
        invalid_max_num_vdevs);
    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    ret = val_host_rmi_granule_range_undelegate(pdev, pdev + PAGE_SIZE);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "Failed to undelegate PDEV granule, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
    }
}
