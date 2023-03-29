/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __MM_COMMON_HOST__
#define __MM_COMMON_HOST__

#include "test_database.h"
#include "val_host_framework.h"
#include "val_host_rmi.h"


uint32_t val_host_init_ripas_delegate(val_host_realm_ts *realm,
                                            val_data_create_ts *data_create);
uint32_t validate_rec_exit_da(val_host_rec_exit_ts *rec_exit, uint64_t hpfar, uint32_t dfsc);
uint32_t validate_rec_exit_ia(val_host_rec_exit_ts *rec_exit, uint64_t hpfar);
#endif /* #ifndef __MM_COMMON_HOST__ */
