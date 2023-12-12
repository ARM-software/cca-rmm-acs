/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_FRAMEWORK_H_
#define _VAL_FRAMEWORK_H_

#include "val.h"
#include "val_rmm.h"

typedef struct _val_print_rsi_host_call {
    SET_MEMBER(struct {
        /* Immediate value */
        unsigned int imm;		/* Offset 0 */
        /* Registers */
        unsigned long gprs[31];
        }, 0, 0x100);
} val_print_rsi_host_call_t;

void val_set_security_state_flag(uint64_t state);
void val_set_running_in_realm_flag(void);
void *val_get_shared_region_base(void);
void *val_get_shared_region_base_pa(void);
void *val_get_shared_region_base_ipa(uint64_t ipa_widt);
uint64_t val_get_ns_shared_region_base_ipa(uint64_t ipa_width, uint64_t pa);
void val_common_printf(const char *msg, uint64_t data1, uint64_t data2);
uint32_t val_printf(const char *msg, uint64_t data1, uint64_t data2);
uint32_t val_get_curr_test_num(void);
void val_set_curr_test_num(uint32_t test_num);
void val_set_status(uint32_t status);
uint32_t val_get_status(void);
uint32_t val_is_current_test(char *testname);
void val_set_curr_test_name(char *testname);
uint32_t val_nvm_write(uint32_t offset, void *buffer, size_t size);
uint32_t val_nvm_read(uint32_t offset, void *buffer, size_t size);
uint32_t val_watchdog_enable(void);
uint32_t val_watchdog_disable(void);
void val_ns_wdog_enable(uint32_t ms);
void val_ns_wdog_disable(void);
uint32_t val_twdog_enable(uint32_t ms);
uint32_t val_twdog_disable(void);
void *val_get_ns_uart_base(void);
void *val_get_secure_base(void);
#endif /* _VAL_FRAMEWORK_H_ */
