/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_HOST_FRAMEWORK_H_
#define _VAL_HOST_FRAMEWORK_H_

#include "val.h"
#include "val_framework.h"
#include "val_mmu.h"
#include "val_host_alloc.h"
#include "val_host_mp.h"

void acs_host_entry(void);
uint64_t val_host_get_secondary_cpu_entry(void);
void val_host_main(bool primary_cpu_boot);
uint32_t val_host_execute_secure_payload(void);
uint32_t val_host_realm_printf_msg_service(void);
void val_host_set_reboot_flag(void);
uint32_t val_host_get_last_run_test_info(val_test_info_ts *test_info);
#endif /* _VAL_HOST_FRAMEWORK_H_ */
