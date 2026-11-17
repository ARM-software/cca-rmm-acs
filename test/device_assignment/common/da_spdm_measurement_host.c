/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "da_spdm_measurement_host.h"

/**
 * @brief Read a little-endian 16-bit value from an SPDM message.
 * @param data Pointer to the first byte of the value.
 * @return The decoded value.
 */
static uint16_t read_u16(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8U);
}

/**
 * @brief Read a little-endian 32-bit value from an SPDM message.
 * @param data Pointer to the first byte of the value.
 * @return The decoded value.
 */
static uint32_t read_u32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8U) |
           ((uint32_t)data[2] << 16U) | ((uint32_t)data[3] << 24U);
}

/**
 * @brief Get the signature size for a negotiated SPDM asymmetric algorithm.
 * @param algorithm Selected SPDM base asymmetric algorithm bit.
 * @return Signature size in bytes, or zero for an unsupported value.
 */
static size_t signature_length(uint32_t algorithm)
{
    switch (algorithm) {
    case 1U << 0U:
    case 1U << 1U:
        return 256U;
    case 1U << 2U:
    case 1U << 3U:
        return 384U;
    case 1U << 4U:
    case 1U << 9U:
    case 1U << 10U:
        return 64U;
    case 1U << 5U:
    case 1U << 6U:
        return 512U;
    case 1U << 7U:
        return 96U;
    case 1U << 8U:
        return 132U;
    case 1U << 11U:
        return 114U;
    default:
        return 0U;
    }
}

/**
 * @brief Read measurement capabilities and algorithms from the cached SPDM VCA.
 * @param pdev PDEV containing the cached negotiation messages.
 * @param info Receives the SPDM version, signing support, and signature size.
 * @return VAL_SUCCESS, VAL_ERROR for invalid data, or VAL_SKIP_CHECK for
 *         an unsupported version or signature algorithm.
 */
uint32_t da_spdm_get_measurement_info(const val_host_pdev_ts *pdev,
                                      da_spdm_measurement_info_ts *info)
{
    const uint8_t *vca = pdev->vca;
    size_t len = pdev->vca_len;
    size_t offset, message_len;
    uint8_t meas_cap;

    if ((vca == NULL) || (len < 10U) ||
        (vca[0] != 0x10U) || (vca[1] != SPDM_GET_VERSION_CODE) ||
        (vca[5] != SPDM_VERSION_CODE))
        return VAL_ERROR;

    offset = 4U;
    message_len = 6U + 2U * (size_t)vca[offset + 5U];
    if (message_len > len - offset)
        return VAL_ERROR;
    offset += message_len;

    if ((len - offset < 12U) ||
        (vca[offset + 1U] != SPDM_GET_CAPABILITIES_CODE))
        return VAL_ERROR;
    info->version = vca[offset];
    if ((info->version < 0x10U) || (info->version > 0x12U))
        return VAL_SKIP_CHECK;

    message_len = (info->version >= 0x12U) ? 20U : 12U;
    if (len - offset < 2U * message_len)
        return VAL_ERROR;
    offset += message_len;
    if ((vca[offset] != info->version) ||
        (vca[offset + 1U] != SPDM_CAPABILITIES_CODE))
        return VAL_ERROR;
    meas_cap = (vca[offset + 8U] >> 3U) & 0x3U;
    offset += message_len;

    if ((len - offset < 6U) ||
        (vca[offset] != info->version) ||
        (vca[offset + 1U] != SPDM_NEGOTIATE_ALGORITHMS_CODE))
        return VAL_ERROR;
    message_len = read_u16(vca + offset + 4U);
    if ((message_len < 16U) || (message_len > len - offset))
        return VAL_ERROR;
    offset += message_len;

    if ((len - offset < 20U) ||
        (vca[offset] != info->version) ||
        (vca[offset + 1U] != SPDM_ALGORITHMS_CODE))
        return VAL_ERROR;
    message_len = read_u16(vca + offset + 4U);
    if ((message_len < 20U) || (message_len != len - offset))
        return VAL_ERROR;

    if ((meas_cap != SPDM_UNSIGNED_MEAS_CAP) &&
        (meas_cap != SPDM_SIGNED_MEAS_CAP))
        return VAL_ERROR;

    info->signed_supported = (meas_cap == SPDM_SIGNED_MEAS_CAP);
    if (info->signed_supported) {
        info->signature_len = signature_length(read_u32(vca + offset + 12U));
        if (info->signature_len == 0U)
            return VAL_SKIP_CHECK;
    }
    info->cert_slot = pdev->cert_slot_id;

    if (info->version != 0x12U)
        return VAL_SKIP_CHECK;

    return VAL_SUCCESS;
}

/**
 * @brief Parse cached SPDM measurement requests and responses in order.
 * @param cache Buffer containing consecutive request and response pairs.
 * @param cache_len Number of bytes in the cache buffer.
 * @param info Negotiated SPDM version, signing support, and signature size.
 * @param check Optional callback to inspect each parsed message.
 * @param context Context passed to the callback.
 * @param pair_count Receives the number of complete pairs parsed.
 * @return VAL_SUCCESS if all pairs are valid, otherwise VAL_ERROR.
 */
uint32_t da_spdm_check_measurement_cache(const uint8_t *cache, size_t cache_len,
                          const da_spdm_measurement_info_ts *info,
                          da_spdm_measurement_message_check_fn check,
                          void *context, uint32_t *pair_count)
{
    const uint8_t *request, *response;
    size_t offset = 0U;
    size_t remaining, request_len, response_len, record_len, opaque_len;
    bool signed_request;

    *pair_count = 0U;
    if ((cache == NULL) || (cache_len == 0U))
        return VAL_ERROR;

    while (offset < cache_len) {
        remaining = cache_len - offset;
        request = cache + offset;
        if ((remaining < 4U) || (request[0] != info->version) ||
            (request[1] != SPDM_GET_MEASUREMENTS_CODE))
            return VAL_ERROR;

        signed_request = (request[2] & 1U) != 0U;
        if (signed_request && !info->signed_supported)
            return VAL_ERROR;
        request_len = signed_request ? SPDM_SIGNED_REQUEST_SIZE : 4U;
        if (request_len > remaining)
            return VAL_ERROR;
        if ((check != NULL) &&
            (check(context, false, request, request_len) != VAL_SUCCESS))
            return VAL_ERROR;
        offset += request_len;

        remaining = cache_len - offset;
        response = cache + offset;
        if ((remaining < SPDM_MEASUREMENT_PREFIX_SIZE) ||
            (response[0] != info->version) ||
            (response[1] != SPDM_MEASUREMENTS_CODE))
            return VAL_ERROR;

        record_len = (size_t)response[5] | ((size_t)response[6] << 8U) |
                     ((size_t)response[7] << 16U);
        if (record_len > remaining - SPDM_MEASUREMENT_PREFIX_SIZE)
            return VAL_ERROR;
        opaque_len = read_u16(response + 40U + record_len);
        if (opaque_len > remaining - SPDM_MEASUREMENT_PREFIX_SIZE - record_len)
            return VAL_ERROR;
        response_len = SPDM_MEASUREMENT_PREFIX_SIZE + record_len + opaque_len;
        if (signed_request) {
            if (info->signature_len > remaining - response_len)
                return VAL_ERROR;
            response_len += info->signature_len;
        }

        if ((check != NULL) &&
            (check(context, true, response, response_len) != VAL_SUCCESS))
            return VAL_ERROR;
        offset += response_len;
        (*pair_count)++;
    }

    return VAL_SUCCESS;
}
