/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"
#include "val_realm_framework.h"

void gic_lr_hw_rejected_realm(void)
{
    val_realm_return_to_host();
}
