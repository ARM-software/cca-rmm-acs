/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"
#include "pal_arch_helpers.h"
#include "val_realm_framework.h"

#define GIC_ENTRY_VMCR_VPMR_VALUE 0xa0U

void gic_rec_entry_gic_state_preserved_realm(void)
{
    write_icc_pmr_el1(GIC_ENTRY_VMCR_VPMR_VALUE);
    isb();

    val_realm_return_to_host();
}
