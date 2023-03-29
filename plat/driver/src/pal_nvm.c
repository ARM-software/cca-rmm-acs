/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_interfaces.h"
#include "pal_nvm.h"

static addr_t nvm_base = PLATFORM_NVM_BASE;

uint32_t pal_driver_nvm_write(uint32_t offset, void *buffer, size_t size)
{
    size_t b_cnt;

    for (b_cnt = 0; b_cnt < size; b_cnt++)
    {
        pal_mmio_write8(nvm_base + offset + b_cnt, *((uint8_t *)buffer + b_cnt));
    }

    return PAL_SUCCESS;
}

uint32_t pal_driver_nvm_read(uint32_t offset, void *buffer, size_t size)
{
    size_t b_cnt;

    for (b_cnt = 0; b_cnt < size; b_cnt++)
    {
        *((uint8_t *)buffer + b_cnt) = pal_mmio_read8(nvm_base + offset + b_cnt);
    }

    return PAL_SUCCESS;
}
