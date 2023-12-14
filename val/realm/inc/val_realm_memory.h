/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_REALM_MEMORY_H_
#define _VAL_REALM_MEMORY_H_

#include "val.h"
#include "val_framework.h"
#include "val_mmu.h"


#include "xlat_tables_v2.h"
#define REALM_MEM_REGIONS 28UL

#define REALM_MAX_VA_IPA_WIDTH 48U
#define REALM_MAX_VIRT_ADDR_SPACE_SIZE (1UL << REALM_MAX_VA_IPA_WIDTH)
#define REALM_MAX_PHY_ADDR_SPACE_SIZE  (1UL << REALM_MAX_VA_IPA_WIDTH)


#define ACS_REALM_CTX_MAX_XLAT_TABLES 30
#ifndef ACS_REALM_IMAGE_XLAT_SECTION_NAME
#define ACS_REALM_IMAGE_XLAT_SECTION_NAME	"xlat_static_tables"
#endif
#ifndef ACS_REALM_IMAGE_BASE_XLAT_SECTION_NAME
#define ACS_REALM_IMAGE_BASE_XLAT_SECTION_NAME	".bss"
#endif

void val_realm_add_mmap(void);
xlat_ctx_t *val_realm_get_xlat_ctx(void);
void val_realm_xlat_add_mmap(void);
int val_realm_pgt_create(val_memory_region_descriptor_ts *mem_desc);
void val_realm_update_xlat_ctx_ias_oas(uint64_t ias, uint64_t oas);
void val_realm_read_attributes(uint64_t va, uint32_t *attr);
int val_realm_update_attributes(uint64_t size, uint64_t va, uint32_t attr);

#endif /* _VAL_REALM_MEMORY_H_ */
