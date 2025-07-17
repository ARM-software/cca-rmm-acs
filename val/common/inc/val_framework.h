/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_FRAMEWORK_H_
#define _VAL_FRAMEWORK_H_

#include "val.h"
#include "val_rmm.h"
#include "val_common_log.h"
#include "val_common_status.h"
#include "val_common_framework.h"
#include "val_common_peripherals.h"

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
void *val_get_shared_region_base_ipa(uint64_t ipa_width);
uint64_t val_get_ns_shared_region_base_ipa(uint64_t ipa_width, uint64_t pa);
void val_realm_printf(print_verbosity_t verbosity, const char *fmt, ...);
uint32_t val_get_curr_test_num(void);
void val_set_curr_test_num(uint32_t test_num);
uint32_t val_is_current_test(char *testname);
void val_set_curr_test_name(char *testname);
void val_ns_wdog_enable(uint32_t ms);
void val_ns_wdog_disable(void);
uint32_t val_twdog_enable(uint32_t ms);
uint32_t val_twdog_disable(void);
void *val_get_ns_uart_base(void);
void *val_get_secure_base(void);
void val_print_secuity_state(void);

#endif /* _VAL_FRAMEWORK_H_ */
