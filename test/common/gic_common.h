/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef GIC_COMMON_H
#define GIC_COMMON_H

#include "test_database.h"

#ifdef ACS_REALM_BUILD
uint32_t gic_realm_counter_equal_check(void);
uint32_t gic_realm_counter_check(void);
void gic_realm_el1_timer_fire(void);
#else
#include "val_host_rmi.h"
#include "val_host_realm.h"

uint32_t gic_host_expect_sysreg_exit(val_host_realm_ts * realm);
uint64_t gic_lr(uint32_t intid, uint32_t state);
uint64_t gic_lr_prio(uint32_t intid, uint32_t state, uint32_t prio);
void gic_host_ich_clear(void);
void gic_host_ich_program(uint64_t hcr, uint64_t vmcr, uint64_t lr0);
void gic_host_ich_write_ap0r(uint32_t index, uint64_t val);
void gic_host_ich_write_ap0r0(uint64_t val);
void gic_host_ich_write_ap1r(uint32_t index, uint64_t val);
void gic_host_ich_write_ap1r0(uint64_t val);
void gic_host_ich_write_hcr(uint64_t val);
void gic_host_ich_write_lr(uint32_t index, uint64_t val);
void gic_host_ich_write_vmcr(uint64_t val);
uint32_t gic_host_ich_num_aprs(void);
uint64_t gic_host_ich_read_ap0r(uint32_t index);
uint64_t gic_host_ich_read_ap0r0(void);
uint64_t gic_host_ich_read_ap1r(uint32_t index);
uint64_t gic_host_ich_read_ap1r0(void);
uint64_t gic_host_ich_read_hcr(void);
uint32_t gic_host_ich_num_lrs(void);
uint64_t gic_host_ich_read_lr(uint32_t index);
uint64_t gic_host_ich_read_lr0(void);
uint64_t gic_host_ich_read_vmcr(void);
#endif

#endif /* GIC_COMMON_H */
