/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"
#include "val_exceptions.h"
#include "val_realm_memory.h"

void exception_non_emulatable_da_realm(void)
{
    val_realm_rsi_host_call_t *gv_realm_host_call;
    val_memory_region_descriptor_ts mem_desc;
    uint64_t ipa_base;
    uint64_t *addr;
    uint32_t val;

    /* Below code is executed for REC[0] only */
    LOG(DBG, "\tIn realm_create_realm REC[0], mpdir=%x\n", val_read_mpidr(), 0);
    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    ipa_base = gv_realm_host_call->gprs[1];

    mem_desc.virtual_address = ipa_base;
    mem_desc.physical_address = ipa_base;
    mem_desc.length = PAGE_SIZE;
    mem_desc.attributes = MT_RW_DATA | MT_REALM;
    if (val_realm_pgt_create(&mem_desc))
    {
        LOG(ERROR, "\tVA to PA mapping failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    addr = (uint64_t *)ipa_base;
    /* Non-emulatable Data Abort: Unprotected IPA whose HIPAS is ASSIGNED_NS,
     * where the access caused a stage 2 permission fault and
     * caused ESR_EL2.ISS.ISV to be set to '0'
     */
    asm volatile("ldxr %0, %1" : "=r" (val) : "Qo" (*(volatile uint32_t *)addr));

exit:
    val_realm_return_to_host();
}
