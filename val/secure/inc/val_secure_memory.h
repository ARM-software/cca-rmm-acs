/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_SECURE_MEMORY_H_
#define _VAL_SECURE_MEMORY_H_

#include "val.h"
#include "val_framework.h"
#include "val_mmu.h"
#include "val_mp_supp.h"

#include "xlat_tables_v2.h"

#define SECURE_MEM_REGIONS 12

#define ACS_SECURE_CTX_MAX_XLAT_TABLES 10
#ifndef ACS_SECURE_IMAGE_XLAT_SECTION_NAME
#define ACS_SECURE_IMAGE_XLAT_SECTION_NAME	"xlat_static_tables"
#endif
#ifndef ACS_SECURE_IMAGE_BASE_XLAT_SECTION_NAME
#define ACS_SECURE_IMAGE_BASE_XLAT_SECTION_NAME	".bss"
#endif

void val_secure_add_mmap(void);
xlat_ctx_t *val_secure_get_xlat_ctx(void);
#endif /* _VAL_SECURE_MEMORY_H_ */
