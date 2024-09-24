/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_planes.h"
#include "val_realm_framework.h"
#include "val_realm_memory.h"
#include "val_hvc.h"

/* Test IPA memory layout
 *
 * TEST IPA : 0x1000 (2nd entry in L3 table)
 * ----------------------------------------
 * | CODE_PAGE, ASSIGNED,RAM  RO+upX      |
 * ----------------------------------------
 * | DATA_PAGE, ASSIGNED,RAM  NoAccess    |
 * ---------------------------------------
 * ipa: 0x3000
 *
 *          (........)
 *
 */

void planes_s2ap_protected_realm(void)
{
    val_realm_rsi_host_call_t *gv_realm_host_call;
    val_memory_region_descriptor_ts mem_desc;
    uint64_t ipa_base;
    uint64_t esr;
    volatile uint32_t *code_page;
    uint64_t cookie_value = 0, base, top;
    val_smc_param_ts cmd_ret;

    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_plane_run_ts run_ptr = {0};

    /* Request Host the test IPA */
    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    ipa_base = gv_realm_host_call->gprs[1];

    /* Map IPA as DATA in Stage 1 */
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

    /* Prepare a page containing following instructions
     * LDR x1, [x0]  opcode: 0xf9400001
     * HVC           opcode: 0xd4000002 */
    code_page = (uint32_t *)ipa_base;
    *code_page = 0xf9400001;
    code_page++;
    *code_page = 0xd4000002;

    /* Give R0+upx permission to code page */
    cmd_ret = val_realm_rsi_mem_set_perm_value(PLANE_1_INDEX, PLANE_1_PERMISSION_INDEX,
                                                                             S2_AP_RO_upX);
    if (cmd_ret.x0) {
        LOG(ERROR, "MEM_SET_PERM_VALUE failed with : %d \n", cmd_ret.x0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    base = ipa_base;
    top = ipa_base + PAGE_SIZE;

    while (base != top) {
        cmd_ret = val_realm_rsi_mem_set_perm_index(base, top, PLANE_1_PERMISSION_INDEX,
                                                                             cookie_value);
        if (cmd_ret.x0 == RSI_ERROR_INPUT || cmd_ret.x2 == RSI_REJECT)
        {
            LOG(ERROR, "MEM_SET_PERM_INDEX failed with : 0x%lx , Response %d \n",
                                                             cmd_ret.x0, cmd_ret.x2);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }

        base = cmd_ret.x1;
        cookie_value = cmd_ret.x3;
    }

    /* Run Plane with x0 as data page*/
    run_ptr.enter.pc = ipa_base;
    run_ptr.enter.gprs[0] = ipa_base + PAGE_SIZE;

    if (val_realm_run_plane(PLANE_1_INDEX, &run_ptr))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    esr = run_ptr.exit.esr_el2;

    /* Check that Plane exit was due permission fault as data page has NoAccess permissions at
     * realm activation */
    if (run_ptr.exit.reason != RSI_EXIT_SYNC      ||
        ESR_EL2_EC(esr) != ESR_EL2_EC_DATA_ABORT       ||
        run_ptr.exit.far_el2 != ipa_base + PAGE_SIZE   ||
        ESR_EL2_ABORT_FSC(esr) != ESR_EL2_ABORT_FSC_PERMISSION_FAULT_L3)
    {
        LOG(ERROR, "Invalid exit type: %d, ESR: 0x%lx",
                                             run_ptr.exit.reason, run_ptr.exit.esr_el2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}

