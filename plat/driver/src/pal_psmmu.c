/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "pal_psmmu.h"
#include "pal_arm_gic.h"
#include "pal_config_def.h"
#include "pal_mmio.h"

#define SMMU_GBPA_OFFSET      0x44
#define SMMU_GBPA_ABORT       (1U << 20)
#define SMMU_GBPA_UPDATE      (1U << 31)

int pal_psmmu_trigger_irq(uint32_t irq_num)
{
    if (!IS_SPI(irq_num)) {
        return -1;
    }

    /* Best-effort register-based trigger for FVP */
    pal_mmio_write32(PSMMU_FVP_BASE_ADDR + SMMU_GBPA_OFFSET,
                     SMMU_GBPA_ABORT | SMMU_GBPA_UPDATE);

    arm_gic_intr_enable(irq_num);
    arm_gic_intr_set_pending(irq_num);
    return 0;
}
