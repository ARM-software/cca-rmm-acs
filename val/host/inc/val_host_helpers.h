/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __VAL_HOST_HELPERS__
#define __VAL_HOST_HELPERS__

#include "test_database.h"
#include "val_host_framework.h"
#include "val_host_rmi.h"

#define GRANULE_SHIFT   12
#define S2TTE_STRIDE    (GRANULE_SHIFT - 3)

typedef enum _exception_da_abort_type {
    EMULATABLE_DA,
    NON_EMULATABLE_DA
} exception_da_abort_type_te;

uint32_t validate_rec_exit_da(val_host_rec_exit_ts *rec_exit, uint64_t hpfar,
                            uint32_t dfsc, exception_da_abort_type_te abort_type, uint32_t wnr);
uint32_t validate_rec_exit_ia(val_host_rec_exit_ts *rec_exit, uint64_t hpfar);
uint64_t val_host_addr_align_to_level(uint64_t addr, uint64_t level);

#endif /* #ifndef __VAL_HOST_HELPERS__ */

