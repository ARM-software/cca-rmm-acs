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
    NON_ATS_PLANE_INVALID = 0X1,
    FRESHNESS_MISMATCH = 0X2,
    VDEV_STATE_NOT_STARTED = 0X3
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
    .abi = RSI_VDEV_DMA_ENABLE,
    .label = VDEV_IDS_INVALID_VDEV_ID,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "non_ats_plane",
    .abi = RSI_VDEV_DMA_ENABLE,
    .label = NON_ATS_PLANE_INVALID,
    .status = RSI_ERROR_INPUT,
    .index = 0},
    {.msg = "freshness",
    .abi = RSI_VDEV_DMA_ENABLE,
    .label = FRESHNESS_MISMATCH,
    .status = RSI_ERROR_DEVICE,
    .index = 0},
    {.msg = "vdev_state",
    .abi = RSI_VDEV_DMA_ENABLE,
    .label = VDEV_STATE_NOT_STARTED,
    .status = RSI_ERROR_DEVICE,
    .index = 0}
};
