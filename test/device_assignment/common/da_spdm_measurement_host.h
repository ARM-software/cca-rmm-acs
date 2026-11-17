/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DA_SPDM_MEASUREMENT_HOST_H
#define DA_SPDM_MEASUREMENT_HOST_H

#include "val_host_da.h"

#define SPDM_GET_VERSION_CODE            0x84U
#define SPDM_VERSION_CODE                0x04U
#define SPDM_GET_CAPABILITIES_CODE       0xE1U
#define SPDM_CAPABILITIES_CODE           0x61U
#define SPDM_NEGOTIATE_ALGORITHMS_CODE   0xE3U
#define SPDM_ALGORITHMS_CODE             0x63U
#define SPDM_GET_MEASUREMENTS_CODE       0xE0U
#define SPDM_MEASUREMENTS_CODE           0x60U
#define SPDM_UNSIGNED_MEAS_CAP           1U
#define SPDM_SIGNED_MEAS_CAP             2U
#define SPDM_SIGNED_REQUEST_SIZE         37U
#define SPDM_MEASUREMENT_PREFIX_SIZE     42U

typedef struct {
    uint8_t version;
    uint8_t cert_slot;
    bool signed_supported;
    size_t signature_len;
} da_spdm_measurement_info_ts;

typedef uint32_t (*da_spdm_measurement_message_check_fn)(void *context,
                                    bool is_response, const uint8_t *data,
                                    size_t len);

uint32_t da_spdm_get_measurement_info(const val_host_pdev_ts *pdev,
                                      da_spdm_measurement_info_ts *info);

/* Parse complete request/response pairs; check can be NULL. */
uint32_t da_spdm_check_measurement_cache(const uint8_t *cache, size_t cache_len,
                          const da_spdm_measurement_info_ts *info,
                          da_spdm_measurement_message_check_fn check,
                          void *context, uint32_t *pair_count);

#endif /* DA_SPDM_MEASUREMENT_HOST_H */
