/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/* This file is derived from xlat_table_v2 library in TF-A project */

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <xlat_tables_defs.h>
#include <xlat_tables_v2.h>
#include <val.h>
#include "xlat_tables_private.h"

#if LOG_LEVEL < LOG_LEVEL_VERBOSE

void xlat_mmap_print(__unused const mmap_region_t *mmap)
{
	/* Empty */
}

void xlat_tables_print(__unused xlat_ctx_t *ctx)
{
	/* Empty */
}

#else /* if LOG_LEVEL >= LOG_LEVEL_VERBOSE */

void xlat_mmap_print(const mmap_region_t *mmap)
{
	LOG(DBG, "mmap:\n", 0, 0);
	const mmap_region_t *mm = mmap;

	while (mm->size != 0U) {
		LOG(DBG, " VA:0x%lx  PA:0x%lx", mm->base_va, mm->base_pa);
                LOG(DBG, " size:0x%lx  attr:0x%x", mm->size, mm->attr);
                LOG(DBG, "  granularity:0x%lx\n", mm->granularity, 0);
		++mm;
	};
	LOG(DBG, "\n", 0, 0);
}

/* Print the attributes of the specified block descriptor. */
static void xlat_desc_print(const xlat_ctx_t *ctx, uint64_t desc)
{
	uint64_t mem_type_index = ATTR_INDEX_GET(desc);
	int xlat_regime = ctx->xlat_regime;

	if (mem_type_index == ATTR_IWBWA_OWBWA_NTR_INDEX) {
		LOG(DBG, "MEM", 0, 0);
	} else if (mem_type_index == ATTR_NON_CACHEABLE_INDEX) {
		LOG(DBG, "NC", 0, 0);
	} else {
		assert(mem_type_index == ATTR_DEV_INDEX);
		LOG(DBG, "DEV", 0, 0);
	}

	if ((xlat_regime == EL3_REGIME) || (xlat_regime == EL2_REGIME)) {
		/* For EL3 and EL2 only check the AP[2] and XN bits. */
		LOG(DBG, ((desc & LOWER_ATTRS(AP_RO)) != 0ULL) ? "-RO" : "-RW", 0, 0);
		LOG(DBG, ((desc & UPPER_ATTRS(XN)) != 0ULL) ? "-XN" : "-EXEC", 0, 0);
	} else {
		assert(xlat_regime == EL1_EL0_REGIME);
		/*
		 * For EL0 and EL1:
		 * - In AArch64 PXN and UXN can be set independently but in
		 *   AArch32 there is no UXN (XN affects both privilege levels).
		 *   For consistency, we set them simultaneously in both cases.
		 * - RO and RW permissions must be the same in EL1 and EL0. If
		 *   EL0 can access that memory region, so can EL1, with the
		 *   same permissions.
		 */
		uint64_t xn_mask = xlat_arch_regime_get_xn_desc(EL1_EL0_REGIME);
		uint64_t xn_perm = desc & xn_mask;

		assert((xn_perm == xn_mask) || (xn_perm == 0ULL));
		LOG(DBG, ((desc & LOWER_ATTRS(AP_RO)) != 0ULL) ? "-RO" : "-RW", 0, 0);
		/* Only check one of PXN and UXN, the other one is the same. */
		LOG(DBG, ((desc & UPPER_ATTRS(PXN)) != 0ULL) ? "-XN" : "-EXEC", 0, 0);
		/*
		 * Privileged regions can only be accessed from EL1, user
		 * regions can be accessed from EL1 and EL0.
		 */
		LOG(DBG, ((desc & LOWER_ATTRS(AP_ACCESS_UNPRIVILEGED)) != 0ULL)
			  ? "-USER" : "-PRIV", 0, 0);
	}

	switch (desc & LOWER_ATTRS(EL3_S1_NSE | NS)) {
	case 0ULL:
		LOG(DBG, "-S", 0, 0);
		break;
	case LOWER_ATTRS(NS):
		LOG(DBG, "-NS", 0, 0);
		break;
	case LOWER_ATTRS(EL3_S1_NSE):
		LOG(DBG, "-RT", 0, 0);
		break;
	default: /* LOWER_ATTRS(EL3_S1_NSE | NS) */
	        LOG(DBG, "-RL", 0, 0);
	}

#ifdef __aarch64__
	/* Check Guarded Page bit */
	if ((desc & GP) != 0ULL) {
		LOG(DBG, "-GP", 0, 0);
	}
#endif
}

static const char * const level_spacers[] = {
	"[LV0] ",
	"  [LV1] ",
	"    [LV2] ",
	"      [LV3] "
};

static const char *invalid_descriptors_ommited =
		"(%d invalid descriptors omitted)\n";

/*
 * Recursive function that reads the translation tables passed as an argument
 * and prints their status.
 */
static void xlat_tables_print_internal(xlat_ctx_t *ctx, uintptr_t table_base_va,
		const uint64_t *table_base, unsigned int table_entries,
		unsigned int level)
{
	assert(level <= XLAT_TABLE_LEVEL_MAX);

	uint64_t desc;
	uintptr_t table_idx_va = table_base_va;
	unsigned int table_idx = 0U;
	size_t level_size = XLAT_BLOCK_SIZE(level);

	/*
	 * Keep track of how many invalid descriptors are counted in a row.
	 * Whenever multiple invalid descriptors are found, only the first one
	 * is printed, and a line is added to inform about how many descriptors
	 * have been omitted.
	 */
	uint64_t invalid_row_count = 0;

	while (table_idx < table_entries) {

		desc = table_base[table_idx];

		if ((desc & DESC_MASK) == INVALID_DESC) {

			if (invalid_row_count == 0) {
				LOG(DBG, level_spacers[level], 0, 0);
				LOG(DBG, "VA:0x%lx ", table_idx_va, 0);
                                LOG(DBG, "size:0x%lx\n", level_size, 0);
			}
			invalid_row_count++;

		} else {

			if (invalid_row_count > 1) {
				LOG(DBG, level_spacers[level], 0, 0);
				LOG(DBG, invalid_descriptors_ommited,
				       invalid_row_count - 1, 0);
			}
			invalid_row_count = 0;

			/*
			 * Check if this is a table or a block. Tables are only
			 * allowed in levels other than 3, but DESC_PAGE has the
			 * same value as DESC_TABLE, so we need to check.
			 */
			if (((desc & DESC_MASK) == TABLE_DESC) &&
					(level < XLAT_TABLE_LEVEL_MAX)) {
				/*
				 * Do not print any PA for a table descriptor,
				 * as it doesn't directly map physical memory
				 * but instead points to the next translation
				 * table in the translation table walk.
				 */
				LOG(DBG, level_spacers[level], 0, 0);
                            	LOG(DBG, "VA:0x%lx ", table_idx_va, 0);
                                LOG(DBG, "size:0x%lx\n", level_size, 0);

				uintptr_t addr_inner = desc & TABLE_ADDR_MASK;

				xlat_tables_print_internal(ctx, table_idx_va,
					(uint64_t *)addr_inner,
					XLAT_TABLE_ENTRIES, level + 1U);
			} else {
				LOG(DBG, level_spacers[level], 0, 0);
				LOG(DBG, "VA:0x%lx", table_idx_va, 0);
                                LOG(DBG, " PA:0x%lx size:0x%lx ",
                                         (uint64_t)(desc & TABLE_ADDR_MASK), level_size);
				xlat_desc_print(ctx, desc);
				LOG(DBG, "\n", 0, 0);
			}
		}

		table_idx++;
		table_idx_va += level_size;
	}

	if (invalid_row_count > 1) {
            LOG(DBG, level_spacers[level], 0, 0);
            LOG(DBG, invalid_descriptors_ommited,
				       invalid_row_count - 1, 0);
	}
}

void xlat_tables_print(xlat_ctx_t *ctx)
{
	const char *xlat_regime_str;
	uint64_t used_page_tables;

	if (ctx->xlat_regime == EL1_EL0_REGIME) {
		xlat_regime_str = "1&0";
	} else if (ctx->xlat_regime == EL2_REGIME) {
		xlat_regime_str = "2";
	} else {
		assert(ctx->xlat_regime == EL3_REGIME);
		xlat_regime_str = "3";
	}
	LOG(DBG, "Translation tables state:\n", 0, 0);
	LOG(DBG, "  Xlat regime:     EL", 0, 0);
        LOG(DBG, xlat_regime_str, 0, 0);
        LOG(DBG, "\n", 0, 0);
	LOG(DBG, "  Max allowed PA:  0x%lx\n", ctx->pa_max_address, 0);
	LOG(DBG, "  Max allowed VA:  0x%lx\n", ctx->va_max_address, 0);
	LOG(DBG, "  Max mapped PA:   0x%lx\n", ctx->max_pa, 0);
	LOG(DBG, "  Max mapped VA:   0x%lx\n", ctx->max_va, 0);

	LOG(DBG, "  Initial lookup level: 0x%x\n", ctx->base_level, 0);
	LOG(DBG, "  Entries @initial lookup level: 0x%x\n", ctx->base_table_entries, 0);

#if PLAT_XLAT_TABLES_DYNAMIC
	used_page_tables = 0;
	for (unsigned int i = 0; i < ctx->tables_num; ++i) {
		if (ctx->tables_mapped_regions[i] != 0)
			++used_page_tables;
	}
#else
	used_page_tables = ctx->next_table;
#endif
	LOG(DBG, "  Used %d sub-tables out of %d", used_page_tables, (uint64_t)ctx->tables_num);
	LOG(DBG, "(spare: %d)\n", (uint64_t)ctx->tables_num - used_page_tables, 0);

	xlat_tables_print_internal(ctx, 0U, ctx->base_table,
				   ctx->base_table_entries, ctx->base_level);
}

#endif /* LOG_LEVEL >= LOG_LEVEL_VERBOSE */

/*
 * Do a translation table walk to find the block or page descriptor that maps
 * virtual_addr.
 *
 * On success, return the address of the descriptor within the translation
 * table. Its lookup level is stored in '*out_level'.
 * On error, return NULL.
 *
 * xlat_table_base
 *   Base address for the initial lookup level.
 * xlat_table_base_entries
 *   Number of entries in the translation table for the initial lookup level.
 * virt_addr_space_size
 *   Size in bytes of the virtual address space.
 */
static uint64_t *find_xlat_table_entry(uintptr_t virtual_addr,
				       void *xlat_table_base,
				       unsigned int xlat_table_base_entries,
				       unsigned long long virt_addr_space_size,
				       unsigned int *out_level)
{
	unsigned int start_level;
	uint64_t *table;
	unsigned int entries;

	start_level = GET_XLAT_TABLE_LEVEL_BASE(virt_addr_space_size);

	table = xlat_table_base;
	entries = xlat_table_base_entries;

	for (unsigned int level = start_level;
	     level <= XLAT_TABLE_LEVEL_MAX;
	     ++level) {
		uint64_t idx, desc, desc_type;

		idx = XLAT_TABLE_IDX(virtual_addr, level);
		if (idx >= entries) {
			LOG(ERROR, "Missing xlat table entry at address 0x%lx\n", virtual_addr, 0);
			return NULL;
		}

		desc = table[idx];
		desc_type = desc & DESC_MASK;

		if (desc_type == INVALID_DESC) {
			LOG(DBG, "Invalid entry (memory not mapped)\n", 0, 0);
			return NULL;
		}

		if (level == XLAT_TABLE_LEVEL_MAX) {
			/*
			 * Only page descriptors allowed at the final lookup
			 * level.
			 */
			assert(desc_type == PAGE_DESC);
			*out_level = level;
			return &table[idx];
		}

		if (desc_type == BLOCK_DESC) {
			*out_level = level;
			return &table[idx];
		}

		assert(desc_type == TABLE_DESC);
		table = (uint64_t *)(uintptr_t)(desc & TABLE_ADDR_MASK);
		entries = XLAT_TABLE_ENTRIES;
	}

	/*
	 * This shouldn't be reached, the translation table walk should end at
	 * most at level XLAT_TABLE_LEVEL_MAX and return from inside the loop.
	 */
	assert(false);

	return NULL;
}


static int xlat_get_mem_attributes_internal(const xlat_ctx_t *ctx,
		uintptr_t base_va, uint32_t *attributes, uint64_t **table_entry,
		unsigned long long *addr_pa, unsigned int *table_level)
{
	uint64_t *entry;
	uint64_t desc;
	unsigned int level;
	unsigned long long virt_addr_space_size;

	/*
	 * Sanity-check arguments.
	 */
	assert(ctx != NULL);
	assert(ctx->initialized);
	assert((ctx->xlat_regime == EL1_EL0_REGIME) ||
	       (ctx->xlat_regime == EL2_REGIME) ||
	       (ctx->xlat_regime == EL3_REGIME));

	virt_addr_space_size = (unsigned long long)ctx->va_max_address + 1ULL;
	assert(virt_addr_space_size > 0U);

	entry = find_xlat_table_entry(base_va,
				ctx->base_table,
				ctx->base_table_entries,
				virt_addr_space_size,
				&level);
	if (entry == NULL) {
		LOG(ERROR, "Address 0x%lx is not mapped.\n", base_va, 0);
		return -EINVAL;
	}

	if (addr_pa != NULL) {
		*addr_pa = *entry & TABLE_ADDR_MASK;
	}

	if (table_entry != NULL) {
		*table_entry = entry;
	}

	if (table_level != NULL) {
		*table_level = level;
	}

	desc = *entry;

#if LOG_LEVEL >= LOG_LEVEL_VERBOSE
	LOG(DBG, "Attributes: ", 0, 0);
	xlat_desc_print(ctx, desc);
	LOG(DBG, "\n", 0, 0);
#endif /* LOG_LEVEL >= LOG_LEVEL_VERBOSE */

	assert(attributes != NULL);
	*attributes = 0U;

	uint64_t attr_index = (desc >> ATTR_INDEX_SHIFT) & ATTR_INDEX_MASK;

	if (attr_index == ATTR_IWBWA_OWBWA_NTR_INDEX) {
		*attributes |= MT_MEMORY;
	} else if (attr_index == ATTR_NON_CACHEABLE_INDEX) {
		*attributes |= MT_NON_CACHEABLE;
	} else {
		assert(attr_index == ATTR_DEV_INDEX);
		*attributes |= MT_DEVICE;
	}

	uint64_t ap2_bit = (desc >> AP2_SHIFT) & 1U;

	if (ap2_bit == AP2_RW)
		*attributes |= MT_RW;

	if (ctx->xlat_regime == EL1_EL0_REGIME) {
		uint64_t ap1_bit = (desc >> AP1_SHIFT) & 1U;

		if (ap1_bit == AP1_ACCESS_UNPRIVILEGED)
			*attributes |= MT_USER;
	}

	uint64_t ns_bit = (desc >> NS_SHIFT) & 1U;

	if (ns_bit == 1U)
		*attributes |= MT_NS;

	uint64_t xn_mask = xlat_arch_regime_get_xn_desc(ctx->xlat_regime);

	if ((desc & xn_mask) == xn_mask) {
		*attributes |= MT_EXECUTE_NEVER;
	} else {
		assert((desc & xn_mask) == 0U);
	}

        /* Set MT_AF_CLEAR bit if Access flag is set */
        if (desc >> ACCESS_FLAG_SHIFT & 1U)
            *attributes |= MT_AF_CLEAR;

	return 0;
}


int xlat_get_mem_attributes_ctx(const xlat_ctx_t *ctx, uintptr_t base_va,
				uint32_t *attr)
{
	return xlat_get_mem_attributes_internal(ctx, base_va, attr,
				NULL, NULL, NULL);
}


int xlat_change_mem_attributes_ctx(const xlat_ctx_t *ctx, uintptr_t base_va,
				   size_t size, uint32_t attr)
{
	/* Note: This implementation isn't optimized. */

	assert(ctx != NULL);
	assert(ctx->initialized);

	unsigned long long virt_addr_space_size =
		(unsigned long long)ctx->va_max_address + 1U;
	assert(virt_addr_space_size > 0U);

	if (!IS_PAGE_ALIGNED(base_va)) {
		LOG(DBG, __func__, 0, 0);
                LOG(ERROR, " Address 0x%lx is not aligned on a page boundary.\n", base_va, 0);
		return -EINVAL;
	}

	if (size == 0U) {
		LOG(DBG, __func__, 0, 0);
                LOG(ERROR, " Size is 0\n", 0, 0);
		return -EINVAL;
	}

	if ((size % PAGE_SIZE) != 0U) {
		LOG(DBG, __func__, 0, 0);
                LOG(ERROR, " Sixe 0x%lx is not a multiple of page size.\n", size, 0);
		return -EINVAL;
	}

	if (((attr & MT_EXECUTE_NEVER) == 0U) && ((attr & MT_RW) != 0U)) {
		LOG(DBG, __func__, 0, 0);
                LOG(ERROR, " Mapping memory as read-write and executable not allowed.\n", 0, 0);
		return -EINVAL;
	}

	size_t pages_count = size / PAGE_SIZE;

	LOG(DBG, "Changing memory attributes of 0x%x pages starting from address 0x%lx...\n",
		pages_count, base_va);

	uintptr_t base_va_original = base_va;

	/*
	 * Sanity checks.
	 */
	for (unsigned int i = 0U; i < pages_count; ++i) {
		const uint64_t *entry;
		uint64_t desc, attr_index;
		unsigned int level;

		entry = find_xlat_table_entry(base_va,
					      ctx->base_table,
					      ctx->base_table_entries,
					      virt_addr_space_size,
					      &level);
		if (entry == NULL) {
			LOG(ERROR, "Address 0x%lx is not mapped.\n", base_va, 0);
			return -EINVAL;
		}

		desc = *entry;

		/*
		 * Check that all the required pages are mapped at page
		 * granularity.
		 */
		if (((desc & DESC_MASK) != PAGE_DESC) ||
			(level != XLAT_TABLE_LEVEL_MAX)) {
			LOG(ALWAYS, "Address 0x%lx is not mapped at the right granularity.\n",
			     base_va, 0);
			LOG(ERROR, "Granularity is 0x%lx, should be 0x%lx.\n",
			     XLAT_BLOCK_SIZE(level), PAGE_SIZE);
			return -EINVAL;
		}

		/*
		 * If the region type is device, it shouldn't be executable.
		 */
		attr_index = (desc >> ATTR_INDEX_SHIFT) & ATTR_INDEX_MASK;
		if (attr_index == ATTR_DEV_INDEX) {
			if ((attr & MT_EXECUTE_NEVER) == 0U) {
				LOG(ERROR, "Setting device memory as executable at address 0x%lx.",
				     base_va, 0);
				return -EINVAL;
			}
		}

		base_va += PAGE_SIZE;
	}

	/* Restore original value. */
	base_va = base_va_original;

	for (unsigned int i = 0U; i < pages_count; ++i) {

		uint32_t old_attr = 0U, new_attr;
		uint64_t *entry = NULL;
		unsigned int level = 0U;
		unsigned long long addr_pa = 0ULL;

		(void) xlat_get_mem_attributes_internal(ctx, base_va, &old_attr,
					    &entry, &addr_pa, &level);

		/* Ignore old attributes. Rewrite the descriptor with new attributs */
                new_attr = attr;

		/*
		 * The break-before-make sequence requires writing an invalid
		 * descriptor and making sure that the system sees the change
		 * before writing the new descriptor.
		 */
		*entry = INVALID_DESC;
#if !HW_ASSISTED_COHERENCY
		dccvac((uintptr_t)entry);
#endif
		/* Invalidate any cached copy of this mapping in the TLBs. */
		xlat_arch_tlbi_va(base_va, ctx->xlat_regime);

		/* Ensure completion of the invalidation. */
		xlat_arch_tlbi_va_sync();

		/* Write new descriptor */
		*entry = xlat_desc(ctx, new_attr, addr_pa, level);
#if !HW_ASSISTED_COHERENCY
		dccvac((uintptr_t)entry);
#endif
		base_va += PAGE_SIZE;
	}

	/* Ensure that the last descriptor written is seen by the system. */
	dsbish();

	return 0;
}
