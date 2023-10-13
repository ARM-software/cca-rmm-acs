/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_HOST_MP_H_
#define _VAL_HOST_MP_H_

#include "val_mp_supp.h"
#include "val_host_framework.h"

uint64_t val_host_power_on_cpu(uint32_t target_cpuid);
uint64_t val_host_power_off_cpu(void);
#endif /* _VAL_HOST_MP_H_ */
