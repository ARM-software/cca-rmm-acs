/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_HOST_MEMORY_H_
#define _VAL_HOST_MEMORY_H_

#include "val.h"
#include "val_framework.h"
#include "val_mmu.h"

#include "xlat_tables_v2.h"

#define HOST_MEM_REGIONS 12

#define ACS_HOST_CTX_MAX_XLAT_TABLES 30
#ifndef ACS_HOST_IMAGE_XLAT_SECTION_NAME
#define ACS_HOST_IMAGE_XLAT_SECTION_NAME	"xlat_static_tables"
#endif
#ifndef ACS_HOST_IMAGE_BASE_XLAT_SECTION_NAME
#define ACS_HOST_IMAGE_BASE_XLAT_SECTION_NAME	".bss"
#endif

void val_host_add_mmap(void);
xlat_ctx_t *val_host_get_xlat_ctx(void);
int val_host_pgt_create(val_memory_region_descriptor_ts *mem_desc);
void val_host_read_atributes(uint64_t va, uint32_t *attr);
int val_host_update_attributes(uint64_t size, uint64_t va, uint32_t attr);
#endif /* _VAL_HOST_MEMORY_H_ */
