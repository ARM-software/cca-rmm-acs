/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_alloc.h"
#include "val_host_realm.h"

typedef struct {
    uint64_t base;
    uint64_t size;
} val_host_alloc_region_ts;

static int number_of_regions;

static uint64_t heap_base;
static uint64_t heap_top;
static uint16_t curr_vmid;

/* get vmid */
uint16_t val_host_get_vmid(void)
{
    curr_vmid = curr_vmid + 1;
    return curr_vmid;
}

static int val_is_power_of_2(uint32_t n)
{
    return n && !(n & (n - 1));
}

/**
 * @brief Allocates contiguous memory of requested size(no_of_bytes) and alignment.
 * @param alignment - alignment for the address. It must be in power of 2.
 * @param Size - Size of the region. It must not be zero.
 * @return - Returns allocated memory base address if allocation is successful.
 *           Otherwise returns NULL.
 **/
void *mem_alloc(size_t alignment, size_t size)
{
    uint64_t addr;

    addr = ADDR_ALIGN(heap_base, alignment);
    size += addr - heap_base;

    if ((heap_top - heap_base) < size)
    {
       LOG(ERROR, "Not enough space available\n", 0, 0);
       return NULL;
    }

    heap_base += size;

    return (void *)addr;
}

/**
 * @brief  Initialisation of allocation data structure
 * @param  void
 * @return Void
 **/
void val_host_mem_alloc_init(void)
{
    heap_base = PLATFORM_HEAP_REGION_BASE;
    heap_top = PLATFORM_HEAP_REGION_BASE + PLATFORM_HEAP_REGION_SIZE;
    number_of_regions = 0;
    curr_vmid = 0;
}

/**
 * @brief Allocates contiguous memory of requested size(no_of_bytes) and alignment.
 * @param alignment - alignment for the address. It must be in power of 2.
 * @param Size - Size of the region. It must not be zero.
 * @return - Returns allocated memory base address if allocation is successful.
 *           Otherwise returns NULL.
 **/
void *val_host_mem_alloc(size_t alignment, size_t size)
{
  void *addr = NULL;

  if (size <= 0)
  {
    LOG(ERROR, "size must be non-zero value\n", 0, 0);
    return NULL;
  }

  if (!val_is_power_of_2((uint32_t)alignment))
  {
    LOG(ERROR, "Alignment must be power of 2\n", 0, 0);
    return NULL;
  }

  size += alignment - 1;
  addr = mem_alloc(alignment, size);
  val_host_add_granule(GRANULE_UNDELEGATED, (uint64_t)addr, NULL);

  return addr;
}

/**
 * Free the memory for given memory address
 * Currently acs code is initialisazing from base for every test,
 * the regions data structure is internal and below code only setting to zero
 * not actually freeing memory.
 **/
void val_host_mem_free(void *ptr)
{
  if (!ptr)
    return;

  return;
}
