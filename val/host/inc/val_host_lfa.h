/*
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_HOST_LFA_H_
#define _VAL_HOST_LFA_H_

#include "val.h"
#include "val_framework.h"
#include "val_mmu.h"
#include "val_smc.h"
#include "val_host_alloc.h"
#include "val_host_framework.h"
#include "val_host_realm.h"

/* LFA SMC IDs */
#define LFA_VERSION             0x840002E0
#define LFA_FEATURES            0x840002E1
#define LFA_GET_INFO            0x840002E2
#define LFA_GET_INVENTORY       0x840002E3
#define LFA_PRIME               0x840002E4


/* LFA Return status codes */
#define LFA_SUCCESS             (int64_t)(0)
#define LFA_NOT_SUPPORTED       (int64_t)(-1)
#define LFA_BUSY                (int64_t)(-2)
#define LFA_AUTH_ERROR          (int64_t)(-3)
#define LFA_NO_MEMORY           (int64_t)(-4)
#define LFA_CRITICAL_ERROR      (int64_t)(-5)
#define LFA_DEVICE_ERROR        (int64_t)(-6)
#define LFA_PRECONDITIONS_FAIL  (int64_t)(-7)
#define LFA_INVALID_ARGUMENTS   (int64_t)(-8)
#define LFA_WRONG_STATE         (int64_t)(-9)

val_smc_param_ts val_host_lfa_version(void);
val_smc_param_ts val_host_lfa_features(uint64_t lfa_fid);
val_smc_param_ts val_host_lfa_get_info(uint64_t lfa_info_selector);
val_smc_param_ts val_host_lfa_get_inventory(uint64_t fw_seq_id);
val_smc_param_ts val_host_lfa_prime(uint64_t fw_seq_id);

#endif /* _VAL_HOST_LFA_H_ */
