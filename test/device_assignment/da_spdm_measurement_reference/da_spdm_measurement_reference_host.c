/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "da_spdm_measurement_host.h"
#include "pal_config_def.h"
#include "val_host_alloc.h"
#include "val_host_da.h"
#include "val_host_pcie.h"
#include "val_host_rmi.h"
#include "mbedtls/md.h"

#define SPDM_MEASUREMENT_INDEX        1U
#define SPDM_MEASUREMENT_SPEC_DMTF    1U
#define SPDM_MEASUREMENT_VALUE_TYPE   0x85U
#define SPDM_MEASUREMENT_VALUE_SIZE   32U
#define SPDM_MEASUREMENT_BLOCK_SIZE   (4U + 3U + SPDM_MEASUREMENT_VALUE_SIZE)

typedef struct {
    uint8_t expected[SPDM_MEASUREMENT_VALUE_SIZE];
    uint32_t response_count;
    bool valid;
} measurement_reference_check_ts;

static uint32_t check_measurement_reference(void *context, bool response,
                                            const uint8_t *data, size_t len)
{
    measurement_reference_check_ts *check = context;
    const uint8_t *block;
    size_t record_len, block_len, value_len;

    if (!response)
        return VAL_SUCCESS;

    check->response_count++;
    if ((len < 8U + SPDM_MEASUREMENT_BLOCK_SIZE) ||
        (data[1] != SPDM_MEASUREMENTS_CODE) || (data[4] != 1U)) {
        LOG(ERROR, "Unexpected SPDM measurement response\n");
        check->valid = false;
        return VAL_SUCCESS;
    }

    record_len = (size_t)data[5] | ((size_t)data[6] << 8U) |
                 ((size_t)data[7] << 16U);
    block = data + 8U;
    block_len = (size_t)block[2] | ((size_t)block[3] << 8U);
    value_len = (size_t)block[5] | ((size_t)block[6] << 8U);

    if ((record_len != SPDM_MEASUREMENT_BLOCK_SIZE) ||
        (record_len > len - 8U) ||
        (block[0] != SPDM_MEASUREMENT_INDEX) ||
        (block[1] != SPDM_MEASUREMENT_SPEC_DMTF) ||
        (block_len != 3U + SPDM_MEASUREMENT_VALUE_SIZE) ||
        (block[4] != SPDM_MEASUREMENT_VALUE_TYPE) ||
        (value_len != SPDM_MEASUREMENT_VALUE_SIZE)) {
        LOG(ERROR, "Unexpected SPDM measurement record layout\n");
        check->valid = false;
        return VAL_SUCCESS;
    }

    if (val_memcmp((void *)(block + 7U), check->expected,
                   SPDM_MEASUREMENT_VALUE_SIZE)) {
        LOG(ERROR, "Cached SPDM measurement differs from FVP reference\n");
        check->valid = false;
    }

    return VAL_SUCCESS;
}

void da_spdm_measurement_reference_host(void)
{
    static const char reference[] = PLATFORM_SPDM_MEASUREMENT_REFERENCE;
    const mbedtls_md_info_t *sha256;
    val_host_realm_ts realm;
    val_host_pdev_ts pdev;
    val_host_vdev_ts vdev;
    val_host_pdev_flags_ts pdev_flags;
    val_host_vdev_measure_params_ts *params = NULL;
    measurement_reference_check_ts check;
    da_spdm_measurement_info_ts measurement_info;
    val_smc_param_ts ret;
    uint64_t prepared_vdev;
    uint32_t num_bdf, rp_bdf, result, pair_count;

    val_memset(&realm, 0, sizeof(realm));
    val_memset(&pdev, 0, sizeof(pdev));
    val_memset(&vdev, 0, sizeof(vdev));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));
    val_memset(&check, 0, sizeof(check));
    val_memset(&measurement_info, 0, sizeof(measurement_info));

    if (!val_host_rmm_supports_da()) {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    /* The selected DOE endpoint must use the matching measurements.json. */
    num_bdf = val_pcie_get_num_bdf();
    if ((num_bdf == VAL_ERROR) ||
        val_pcie_find_doe_capability(&num_bdf, &pdev.bdf,
                                     &pdev.doe_cap_base) ||
        val_pcie_get_rootport(pdev.bdf, &rp_bdf)) {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    sha256 = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if ((sha256 == NULL) ||
        mbedtls_md(sha256, (const unsigned char *)reference,
                   sizeof(reference) - 1U, check.expected)) {
        LOG(ERROR, "Could not hash the FVP measurement reference\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    pdev.root_id = (uint16_t)rp_bdf;
    val_host_set_pdev_flags(&pdev_flags);
    pdev.pdev_flags = pdev_flags;
    vdev.vmid = 1U;
    vdev.vdev_id = 1U;
    vdev.rec_count = 1U;

    prepared_vdev = g_vdev_state_prep_sequence(&pdev, &vdev, &realm,
                                               RMI_VDEV_UNLOCKED);
    if (prepared_vdev == VAL_TEST_PREP_SEQ_FAILED) {
        LOG(ERROR, "SPDM VDEV preparation failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto cleanup;
    }
    vdev.vdev = prepared_vdev;

    result = da_spdm_get_measurement_info(&pdev, &measurement_info);
    if (result == VAL_SKIP_CHECK) {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto cleanup;
    }
    if (result != VAL_SUCCESS) {
        LOG(ERROR, "Invalid cached SPDM negotiation data\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto cleanup;
    }

    vdev.meas = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_VDEV_MEAS_LEN_MAX);
    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((vdev.meas == NULL) || (params == NULL)) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto cleanup;
    }
    val_memset(params, 0, PAGE_SIZE);
    vdev.meas_len = 0U;
    check.valid = true;

    ret = val_host_rmi_vdev_get_measurements(realm.rd, pdev.pdev,
                                             vdev.vdev, (uint64_t)params);
    if (ret.x0 != RMI_SUCCESS) {
        LOG(ERROR, "RMI_VDEV_GET_MEASUREMENTS failed, ret=0x%lx\n",
            (unsigned long)ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto cleanup;
    }

    if (val_host_dev_communicate(&realm, &pdev, &vdev, RMI_VDEV_UNLOCKED)) {
        LOG(ERROR, "VDEV measurement communication failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto cleanup;
    }

    result = da_spdm_check_measurement_cache(vdev.meas, vdev.meas_len,
                 &measurement_info, check_measurement_reference, &check,
                 &pair_count);
    if ((result != VAL_SUCCESS) || !check.valid ||
        (pair_count != 1U) || (check.response_count != 1U)) {
        LOG(ERROR, "Cached measurement does not match the FVP reference\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto cleanup;
    }

    LOG(TEST, "Cached SPDM measurement matches the FVP reference\n");
    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    if ((vdev.vdev != 0U) &&
        val_host_vdev_teardown(&realm, &pdev, &vdev)) {
        LOG(ERROR, "VDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
    }
    if ((pdev.pdev != 0U) &&
        (pdev.pdev != VAL_TEST_PREP_SEQ_FAILED) &&
        val_host_pdev_teardown(&pdev, pdev.pdev)) {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
    }
    if (params != NULL)
        val_host_mem_free(params);
    if (vdev.meas != NULL)
        val_host_mem_free(vdev.meas);
}
