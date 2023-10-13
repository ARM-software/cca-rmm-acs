/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_IRQ_H_
#define _VAL_IRQ_H_

#include "val.h"

#define GICV3_LR_HW     61
#define GICV3_LR_STATE  62
#define GICV3_LR_GROUP  60
#define GICV3_LR_pINTID 32

#define GICV3_LR_STATE_INACTIVE             0x0
#define GICV3_LR_STATE_PENDING              0x1
#define GICV3_LR_STATE_ACTIVE               0x2
#define GICV3_LR_STATE_PENDING_AND_ACTIVE   0x3

#define GICV3_HCR_EL2_EN             0
#define GICV3_HCR_EL2_UIE            1
#define GICV3_HCR_EL2_LRENPIE        2
#define GICV3_HCR_EL2_NPIE           3
#define GICV3_HCR_EL2_EOICOUNT       27

#define GICV3_MISR_EL2_U            1
#define GICV3_MISR_EL2_LRENP        2
#define GICV3_MISR_EL2_NP           3

void val_irq_setup(void);
int val_irq_handler_dispatcher(void);
void val_irq_enable(uint32_t irq_num, uint8_t irq_priority);
void val_configure_secure_irq_enable(uint32_t irq_num);
void val_irq_disable(uint32_t irq_num);
int val_irq_register_handler(uint32_t num, void *irq_handler);
int val_irq_unregister_handler(uint32_t irq_num);
void val_gic_end_of_intr(unsigned int irq_num);

#endif /* _VAL_IRQ_H_ */
