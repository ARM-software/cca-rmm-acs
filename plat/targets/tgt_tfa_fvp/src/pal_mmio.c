/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_mmio.h"

/* MMIO read/write access functions */

uint8_t pal_mmio_read8(uint64_t addr)
{
  return *(volatile uint8_t *)addr;
}

uint16_t pal_mmio_read16(uint64_t addr)
{
  return *(volatile uint16_t *)addr;
}

uint64_t pal_mmio_read64(uint64_t addr)
{
  return *(volatile uint64_t *)addr;
}

uint32_t pal_mmio_read32(uint64_t addr)
{
  return *(volatile uint32_t *)addr;
}

void pal_mmio_write8(uint64_t addr, uint8_t data)
{
    *(volatile uint8_t *)addr = data;
}

void pal_mmio_write16(uint64_t addr, uint16_t data)
{
    *(volatile uint16_t *)addr = data;
}

void pal_mmio_write64(uint64_t addr, uint64_t data)
{
    *(volatile uint64_t *)addr = data;
}

void pal_mmio_write32(uint64_t addr, uint32_t data)
{
    *(volatile uint32_t *)addr = data;
}
