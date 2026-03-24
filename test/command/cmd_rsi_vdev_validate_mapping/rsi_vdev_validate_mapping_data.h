/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    VDEV_IDS_INVALID_VDEV_ID = 0X0,
    IPA_BASE_UNALIGNED = 0X1,
    IPA_TOP_UNALIGNED = 0X2,
    PA_UNALIGNED = 0X3,
    SIZE_INVALID = 0X4,
    REGION_UNPROTECTED = 0X5,
    ATTEST_INFO_MISMATCH = 0X6
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data1[] = {
//    {.msg = "vdev_id",
//    .abi = RSI_VDEV_VALIDATE_MAPPING,
//    .label = VDEV_IDS_INVALID_VDEV_ID,
//    .status = RSI_ERROR_INPUT,
//    .index = 0},
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
    .index = 0},
    {.msg = "attest_info",
    .abi = RSI_VDEV_VALIDATE_MAPPING,
    .label = ATTEST_INFO_MISMATCH,
    .status = RSI_ERROR_DEVICE,
    .index = 0}
};
