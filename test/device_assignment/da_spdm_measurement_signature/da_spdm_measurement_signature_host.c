/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "da_spdm_measurement_host.h"
#include "val_host_alloc.h"
#include "val_host_da.h"
#include "val_host_pcie.h"
#include "val_host_rmi.h"

#define SPDM_MAX_OPAQUE_DATA_SIZE        1024U
#define SPDM_NONCE_SIZE                  32U

typedef struct {
    uint8_t version;
    uint8_t cert_slot;
    uint8_t nonce[SPDM_NONCE_SIZE];
    size_t signature_len;
    uint32_t request_count;
    uint32_t response_count;
    uint32_t signed_response_count;
    bool signed_supported;
    bool waiting_for_response;
    bool request_signed;
    bool last_response_signed;
    bool valid;
} measurement_signature_check_ts;

static uint16_t read_u16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8U);
}

static uint32_t check_measurement_signature(void *context, bool response,
                                             const uint8_t *data, size_t len)
{
    measurement_signature_check_ts *check = context;
    size_t record_len, opaque_offset, opaque_len, signature_offset;

    LOG(TEST, "SPDM cache parse: %s, len=%lu\n",
        response ? "response" : "request", (unsigned long)len);

    if ((len < 4U) || (data[0] != check->version) ||
        (data[1] != (response ? SPDM_MEASUREMENTS_CODE :
                                SPDM_GET_MEASUREMENTS_CODE))) {
        LOG(ERROR, "Unexpected SPDM measurement %s: len=%lu, version=0x%x, code=0x%x\n",
            response ? "response" : "request", (unsigned long)len,
            len > 0U ? data[0] : 0U, len > 1U ? data[1] : 0U);
        check->valid = false;
        return VAL_SUCCESS;
    }

    if (!response) {
        if (check->waiting_for_response) {
            LOG(ERROR, "SPDM measurement request arrived before its response\n");
            check->valid = false;
        }
        check->request_signed = (data[2] & 1U) != 0U;
        if (check->request_signed && !check->signed_supported) {
            LOG(ERROR, "Signature requested from an unsigned-only device\n");
            check->valid = false;
        }
        if (check->request_signed &&
            ((len != SPDM_SIGNED_REQUEST_SIZE) || (data[3] != 0xFFU) ||
             ((data[36] & 0xF0U) != 0U) ||
             ((data[36] & 0x0FU) != check->cert_slot) ||
             val_memcmp((void *)(data + 4U), check->nonce,
                        SPDM_NONCE_SIZE))) {
            LOG(ERROR, "Invalid signed SPDM request: len=%lu, operation=0x%x, "
                "slot=0x%x, expected_slot=0x%x, nonce_match=%d\n",
                (unsigned long)len, data[3], len > 36U ? data[36] : 0U,
                check->cert_slot,
                len >= SPDM_SIGNED_REQUEST_SIZE &&
                !val_memcmp((void *)(data + 4U), check->nonce,
                            SPDM_NONCE_SIZE));
            check->valid = false;
        }
        if (!check->request_signed && (len != 4U)) {
            LOG(ERROR, "Unsigned SPDM request has unexpected fields\n");
            check->valid = false;
        }
        check->waiting_for_response = true;
        check->request_count++;
        return VAL_SUCCESS;
    }

    if (!check->waiting_for_response || (len < SPDM_MEASUREMENT_PREFIX_SIZE) ||
        (check->request_signed && ((data[3] & 0x0FU) != check->cert_slot)) ||
        (!check->request_signed && (data[3] != 0U))) {
        LOG(ERROR, "Invalid SPDM response prefix: len=%lu, waiting=%d, "
            "signed=%d, slot=0x%x, expected_slot=0x%x\n",
            (unsigned long)len, check->waiting_for_response,
            check->request_signed, data[3] & 0x0FU, check->cert_slot);
        check->valid = false;
    } else {
        record_len = (size_t)data[5] | ((size_t)data[6] << 8U) |
                     ((size_t)data[7] << 16U);
        if (record_len > len - SPDM_MEASUREMENT_PREFIX_SIZE) {
            LOG(ERROR, "Invalid SPDM measurement record length: record=%lu, response=%lu\n",
                (unsigned long)record_len, (unsigned long)len);
            check->valid = false;
        } else {
            opaque_offset = 40U + record_len;
            opaque_len = read_u16(data + opaque_offset);
            signature_offset = SPDM_MEASUREMENT_PREFIX_SIZE + record_len +
                               opaque_len;
            if ((opaque_len > SPDM_MAX_OPAQUE_DATA_SIZE) ||
                (signature_offset > len) ||
                (len - signature_offset !=
                 (check->request_signed ? check->signature_len : 0U))) {
                LOG(ERROR, "Invalid SPDM response fields: opaque=%lu, "
                    "signature_offset=%lu, response=%lu, expected_signature=%lu\n",
                    (unsigned long)opaque_len,
                    (unsigned long)signature_offset, (unsigned long)len,
                    (unsigned long)(check->request_signed ?
                                    check->signature_len : 0U));
                check->valid = false;
            }
        }
    }

    check->response_count++;
    check->waiting_for_response = false;
    check->last_response_signed = check->request_signed;
    if (check->request_signed) {
        check->signed_response_count++;
        if (check->signed_response_count != 1U) {
            LOG(ERROR, "Multiple signed SPDM measurement responses\n");
            check->valid = false;
        }
    }

    return VAL_SUCCESS;
}

void da_spdm_measurement_signature_host(void)
{
    val_host_realm_ts realm;
    val_host_pdev_ts pdev;
    val_host_vdev_ts vdev;
    val_host_pdev_flags_ts pdev_flags;
    val_host_vdev_measure_params_ts *params = NULL;
    measurement_signature_check_ts check;
    da_spdm_measurement_info_ts measurement_info;
    val_smc_param_ts ret;
    uint64_t prepared_vdev;
    uint32_t num_bdf, rp_bdf, result, pair_count;
    uint32_t i;

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

    num_bdf = val_pcie_get_num_bdf();
    if ((num_bdf == VAL_ERROR) ||
        val_pcie_find_doe_capability(&num_bdf, &pdev.bdf,
                                     &pdev.doe_cap_base) ||
        val_pcie_get_rootport(pdev.bdf, &rp_bdf)) {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
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
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
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
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto cleanup;
    }
    check.version = measurement_info.version;
    check.cert_slot = measurement_info.cert_slot;
    check.signed_supported = measurement_info.signed_supported;
    check.signature_len = measurement_info.signature_len;

    vdev.meas = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_VDEV_MEAS_LEN_MAX);
    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((vdev.meas == NULL) || (params == NULL)) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto cleanup;
    }
    val_memset(params, 0, PAGE_SIZE);
    for (i = 0U; i < SPDM_NONCE_SIZE; i++)
        params->nonce[i] = (uint8_t)(i + 1U);
    val_memcpy(check.nonce, params->nonce, SPDM_NONCE_SIZE);
    vdev.meas_len = 0U;
    check.valid = true;

    ret = val_host_rmi_vdev_get_measurements(realm.rd, pdev.pdev,
                                             vdev.vdev, (uint64_t)params);
    LOG(TEST, "SPDM measurement trace: GET_MEASUREMENTS ret=0x%lx\n",
        (unsigned long)ret.x0);
    if (ret.x0 != RMI_SUCCESS) {
        LOG(ERROR, "RMI_VDEV_GET_MEASUREMENTS failed, ret=0x%lx\n",
            (unsigned long)ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto cleanup;
    }

    if (val_host_dev_communicate(&realm, &pdev, &vdev, RMI_VDEV_UNLOCKED)) {
        LOG(ERROR, "VDEV measurement communication failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto cleanup;
    }

    result = da_spdm_check_measurement_cache(vdev.meas, vdev.meas_len,
                 &measurement_info, check_measurement_signature, &check,
                 &pair_count);
    LOG(TEST, "SPDM measurement trace: parse=%u, pairs=%u\n",
        result, pair_count);
    if ((result != VAL_SUCCESS) || !check.valid ||
        (check.request_count == 0U) ||
        (check.request_count != check.response_count) ||
        (pair_count != check.request_count) || check.waiting_for_response ||
        (check.signed_supported &&
         (!check.last_response_signed || (check.signed_response_count != 1U))) ||
        (!check.signed_supported &&
         (check.last_response_signed || (check.signed_response_count != 0U)))) {
        LOG(ERROR, "SPDM measurement cache: valid=%d, requests=%u, responses=%u, "
            "pending=%d, signed_supported=%d, final_signed=%d, signed_responses=%u\n",
            check.valid, check.request_count, check.response_count,
            check.waiting_for_response, check.signed_supported,
            check.last_response_signed,
            check.signed_response_count);
        LOG(ERROR, "SPDM measurement signature field is missing or unexpected\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto cleanup;
    }

    LOG(TEST, "Cached SPDM response has the expected signature field length: %lu\n",
        (unsigned long)(check.signed_supported ? check.signature_len : 0U));
    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    if ((vdev.vdev != 0U) &&
        val_host_vdev_teardown(&realm, &pdev, &vdev)) {
        LOG(ERROR, "VDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
    }
    if ((pdev.pdev != 0U) &&
        (pdev.pdev != VAL_TEST_PREP_SEQ_FAILED) &&
        val_host_pdev_teardown(&pdev, pdev.pdev)) {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
    }
    if (params != NULL)
        val_host_mem_free(params);
    if (vdev.meas != NULL)
        val_host_mem_free(vdev.meas);
}
