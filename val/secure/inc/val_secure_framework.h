/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_SECURE_FRAMEWORK_H_
#define _VAL_SECURE_FRAMEWORK_H_

#include "val.h"
#include "val_framework.h"
#include "val_mmu.h"
#include "val_secure_memory.h"

void val_secure_main(bool primary_cpu_boot);
uint32_t val_secure_return_to_host(void);
uint32_t val_secure_return_to_preempted_host(void);
#endif /* _VAL_SECURE_FRAMEWORK_H_ */
