/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_TIMER_H_
#define _VAL_TIMER_H_

#include "val.h"
#include "val_framework.h"

#define ARM_ARCH_TIMER_ENABLE           (1ULL << 0)
#define ARM_ARCH_TIMER_IMASK            (1ULL << 1)
#define ARM_ARCH_TIMER_ISTATUS          (1ULL << 2)

void val_disable_phy_timer_el1(void);
void val_timer_set_phy_el1(uint64_t timeout, bool irq_mask);
void val_disable_virt_timer_el1(void);
void val_timer_set_virt_el1(uint64_t timeout);
void val_disable_phy_timer_el2(void);
void val_timer_set_phy_el2(uint64_t timeout);

#endif /* _VAL_TIMER_H_ */
