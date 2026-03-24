/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    VDEV_IDS_INVALID_VDEV_ID = 0X0
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
    .abi = RSI_VDEV_DMA_DISABLE,
    .label = VDEV_IDS_INVALID_VDEV_ID,
    .status = RSI_ERROR_INPUT,
    .index = 0}
};
