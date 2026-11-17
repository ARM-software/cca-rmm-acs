/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"
#include "val_realm_rsi_types.h"

enum test_intent {
    VDEV_IDS_INVALID_VDEV_ID = 0X0,
    VDEV_STATE_NOT_LOCKED_OR_STARTED = 0X1,
    IPA_BASE_UNALIGNED = 0X2,
    IPA_TOP_UNALIGNED = 0X3,
    PA_UNALIGNED = 0X4,
    SIZE_INVALID = 0X5,
    REGION_UNPROTECTED = 0X6
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data1[] = {
    {.msg = "vdev_id",
    .abi = RSI_VDEV_VALIDATE_MAPPING,
    .label = VDEV_IDS_INVALID_VDEV_ID,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "state",
    .abi = RSI_VDEV_VALIDATE_MAPPING,
    .label = VDEV_STATE_NOT_LOCKED_OR_STARTED,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "ipa_base_align",
    .abi = RSI_VDEV_VALIDATE_MAPPING,
    .label = IPA_BASE_UNALIGNED,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "ipa_top_align",
    .abi = RSI_VDEV_VALIDATE_MAPPING,
    .label = IPA_TOP_UNALIGNED,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "pa_align",
    .abi = RSI_VDEV_VALIDATE_MAPPING,
    .label = PA_UNALIGNED,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "size_valid",
    .abi = RSI_VDEV_VALIDATE_MAPPING,
    .label = SIZE_INVALID,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "rgn_bound",
    .abi = RSI_VDEV_VALIDATE_MAPPING,
    .label = REGION_UNPROTECTED,
    .status = RSI_ERROR_INPUT,
    .index = 0}
};
