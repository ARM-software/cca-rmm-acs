/*
 * Copyright (c) 2023, 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_HOST_ALLOC_H_
#define _VAL_HOST_ALLOC_H_

#include "val.h"
#include "val_framework.h"

#define __ADDR_ALIGN_MASK(a, mask)    (((a) + (mask)) & ~(mask))
#define ADDR_ALIGN(a, b)              __ADDR_ALIGN_MASK(a, (typeof(a))(b) - 1)

void val_host_mem_alloc_init(void);
void *val_host_mem_alloc(size_t alignment, size_t size);
void val_host_mem_free(void *ptr);
void *mem_alloc(size_t alignment, size_t size);
uint16_t val_host_get_vmid(void);
void *val_buffer_alloc_calloc(size_t n, size_t size);
void val_buffer_alloc_free(void *ptr);

#endif /* _VAL_HOST_ALLOC_H_ */
