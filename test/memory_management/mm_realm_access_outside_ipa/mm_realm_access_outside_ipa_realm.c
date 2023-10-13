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
#include "val_realm_exception.h"
#include "val_realm_memory.h"

static volatile uint32_t g_ec, g_dfsc;
static volatile uint64_t g_far;
static volatile uint32_t g_handler_flag;

static bool sea_handler(void)
{
    uint64_t esr_el1 = val_esr_el1_read();
    uint64_t far_el1 = val_far_el1_read();
    uint64_t next_pc = val_elr_el1_read() + 4;
    uint64_t ec = esr_el1 >> 26;
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t realm_host_params;

    if ((ec != g_ec) || (g_far != far_el1) || (VAL_EXTRACT_BITS(esr_el1, 0, 5) != g_dfsc))
    {
        realm_host_params.gprs[1] = VAL_ERROR;
        realm_host_params.gprs[2] = esr_el1;
        val_realm_rsi_host_params(&realm_host_params);
    }

    if (ec == EC_INSTRUCTION_ABORT_SAME_EL)
    {
        val_realm_return_to_host();
    }

    /* Skip instruction that triggered the exception. */
    val_elr_el1_write(next_pc);
    g_handler_flag = 1;

    return true;
}

void mm_realm_access_outside_ipa_realm(void)
{
    uint64_t ipa_base;
    uint64_t va = 0x80000;
    uint32_t ipa_width;
    val_memory_region_descriptor_ts mem_desc;
    uint8_t  currentEL;
    uint64_t pa_range = val_get_pa_range_supported();
    void (*fun_ptr)(void);

    currentEL = (val_read_current_el() & 0xc) >> 2;

    /* Below code is executed for REC[0] only */
    LOG(DBG, "\tIn realm_create_realm REC[0], mpdir=%x\n", val_read_mpidr(), 0);

    val_realm_return_to_host();
    val_exception_setup(NULL, sea_handler);

    ipa_width = (uint32_t)val_realm_get_ipa_width();
    ipa_base = 1ULL << (ipa_width);

    val_realm_update_xlat_ctx_ias_oas(((1UL << ipa_width) - 1), ((1UL << (ipa_width + 8)) - 1));

    mem_desc.virtual_address = va;
    mem_desc.physical_address = ipa_base;
    mem_desc.length = PAGE_SIZE;
    mem_desc.attributes = MT_RW_DATA | MT_REALM;
    if (val_realm_pgt_create(&mem_desc))
    {
        LOG(ERROR, "\tVA to PA mapping failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    g_ec = EC_DATA_ABORT_SAME_EL;
    g_far = va;
    g_dfsc = 0x3; /* Address size fault, Level 3 */
    /* If stage 1 translation is enabled, Realm access to an IPA which is greater than
     * the IPA space of the Realm causes a stage 1 Address Size Fault taken to the Realm.
     */
    *(volatile uint32_t *)va = 0x100;

    if (!g_handler_flag)
    {
        LOG(ERROR, "\tData abort not triggered\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    /* Disable MMU */
    val_sctlr_write((~1U) &  // M=1 Enable the stage 1 MMU
                    val_sctlr_read(currentEL),
                    currentEL);

    ipa_base = 1ULL << pa_range;
    g_ec = EC_DATA_ABORT_SAME_EL;
    g_dfsc = 0x0; /* Address size fault, Level 0 */
    g_far = ipa_base;
    /* If stage 1 translation is disabled, Realm access to an IPA which is greater than
     * the IPA space of the Realm causes a stage 1 level 0 Address Size Fault taken to the Realm.
     */
    *(volatile uint32_t *)ipa_base = 0x100;

    g_ec = EC_INSTRUCTION_ABORT_SAME_EL;
    fun_ptr = (void *)ipa_base;
    (*fun_ptr)();

exit:
    val_exception_setup(NULL, NULL);
    val_realm_return_to_host();
}
