/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _PAL_MMIO_H_
#define _PAL_MMIO_H_

#include "pal_interfaces.h"

/* MMIO read/write access functions */

/**
  @brief  Provides a single point of abstraction to read 8 bit data from
          Memory Mapped IO address

  @param  addr 64-bit address

  @return 8-bit data read from the input address
 **/
uint8_t pal_mmio_read8(uint64_t addr);

/**
  @brief  Provides a single point of abstraction to read 16 bit data from
          Memory Mapped IO address

  @param  addr 64-bit address

  @return 16-bit data read from the input address
 **/
uint16_t pal_mmio_read16(uint64_t addr);

/**
  @brief  Provides a single point of abstraction to read 64 bit data from
          Memory Mapped IO address

  @param  addr 64-bit address

  @return 64-bit data read from the input address
**/
uint64_t pal_mmio_read64(uint64_t addr);

/**
  @brief  Provides a single point of abstraction to read 32-bit data from
          Memory Mapped IO address

  @param  addr 64-bit address

  @return 32-bit data read from the input address
**/
uint32_t pal_mmio_read32(uint64_t addr);

/**
  @brief  Provides a single point of abstraction to write 8-bit data to
          Memory Mapped IO address

  @param  addr  64-bit address
  @param  data  8-bit data to write to address

  @return None
**/
void pal_mmio_write8(uint64_t addr, uint8_t data);

/**
  @brief  Provides a single point of abstraction to write 16-bit data to
          Memory Mapped IO address

  @param  addr  64-bit address
  @param  data  16-bit data to write to address

  @return None
**/
void pal_mmio_write16(uint64_t addr, uint16_t data);

/**
  @brief  Provides a single point of abstraction to write 64-bit data to
          Memory Mapped IO address

  @param  addr  64-bit address
  @param  data  64-bit data to write to address

  @return None
**/
void pal_mmio_write64(uint64_t addr, uint64_t data);

/**
  @brief  Provides a single point of abstraction to write 32-bit data to
          Memory Mapped IO address

  @param  addr  64-bit address
  @param  data  32-bit data to write to address

  @return None
**/
void pal_mmio_write32(uint64_t addr, uint32_t data);
#endif  /* _PAL_MMIO_H_ */
