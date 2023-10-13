/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"

void gic_obsv_vmcr_realm(void)
{
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t realm_host_params;

    /* Below code is executed for REC[0] only */
    LOG(DBG, "\tIn realm_create_realm REC[0], mpdir=%x\n", val_read_mpidr(), 0);
    realm_host_params.gprs[1] = read_icv_pmr_el1();
    realm_host_params.gprs[2] = read_icv_ctrl_el1();
    realm_host_params.gprs[3] = read_icv_igrpen1_el1();
    realm_host_params.gprs[4] = read_icv_bpr0_el1();

    val_realm_rsi_host_params(&realm_host_params);
}
