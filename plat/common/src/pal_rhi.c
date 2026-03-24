/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_interfaces.h"

uint64_t pal_rhi_alloc_session_id(void)
{
    static uint64_t session_id = 1;
    return session_id++;
}

