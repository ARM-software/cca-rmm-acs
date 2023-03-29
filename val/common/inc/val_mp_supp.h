/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_MP_SUPP_H_
#define _VAL_MP_SUPP_H_

#include "val_sysreg.h"
#include "val_psci.h"

uint64_t val_get_primary_mpidr(void);
uint32_t val_get_cpu_count(void);
uint32_t val_get_cpuid(uint64_t mpidr);
uint64_t val_get_mpidr(uint32_t cpu_id);
uint64_t val_get_vmpidr(uint32_t cpu_id);

void val_init_spinlock(s_lock_t *lock);
void val_spin_lock(s_lock_t *lock);
void val_spin_unlock(s_lock_t *lock);
void val_init_event(event_t *event);
void val_send_event(event_t *event);
void val_send_event_to_all(event_t *event);
void val_send_event_to(event_t *event, unsigned int cpus_count);
void val_wait_for_event(event_t *event);

#endif /* _VAL_MP_SUPP_H_ */
