/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_REALM_FRAMEWORK_H_
#define _VAL_REALM_FRAMEWORK_H_

#include "val.h"
#include "val_framework.h"
#include "val_mmu.h"
#include "val_psci.h"
#include "val_mp_supp.h"

void val_realm_main(bool primary_cpu_boot);
uint64_t val_realm_get_secondary_cpu_entry(void);
void acs_realm_entry(void);
void val_realm_return_to_host(void);
uint32_t val_realm_printf(const char *msg, uint64_t data1, uint64_t data2);
#endif /* _VAL_REALM_FRAMEWORK_H_ */
