/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef PAL_PSMMU_H
#define PAL_PSMMU_H

#include <stdint.h>

/*
 * Trigger a PSMMU interrupt at the GIC for a given IRQ number.
 * Returns 0 on success, non-zero on failure.
 */
int pal_psmmu_trigger_irq(uint32_t irq_num);

#endif /* PAL_PSMMU_H */
