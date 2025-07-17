/*
 * Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_hvc.h"
#include "val_realm_memory.h"
#include "planes_s2ap_enforced_by_p0_data.h"

void planes_s2ap_enforced_by_p0_realm(void)
{
    val_realm_rsi_host_call_t *gv_realm_host_call;
    val_memory_region_descriptor_ts mem_desc;
    uint64_t ipa_base;
    uint64_t esr;
    volatile uint32_t *code_page;
    uint64_t cookie_value = 0, base, top;
    val_smc_param_ts cmd_ret;
    uint8_t i, j;

    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_plane_run_ts run_ptr = {0};

    /* Request Host the test IPA */
    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    ipa_base = gv_realm_host_call->gprs[1];

    /* Map CODE pages as DATA in Stage 1 as we need to write the instructions into code pages*/
    mem_desc.virtual_address = ipa_base;
    mem_desc.physical_address = ipa_base;
    mem_desc.length = PAGE_SIZE;
    mem_desc.attributes = MT_RW_DATA | MT_REALM;
    for (i = 0; i < CODE_PAGE_COUNT; i++)
    {
        if (val_realm_pgt_create(&mem_desc))
        {
            LOG(ERROR, "VA to PA mapping failed\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto exit;
        }
        mem_desc.virtual_address += PAGE_SIZE;
        mem_desc.physical_address += PAGE_SIZE;
    }

    /* Write required instructions into code page, test header file for more info */
    code_page = (uint32_t *)ipa_base;
    for (i = 0; i < CODE_PAGE_COUNT; i++)
    {
        val_memcpy((void *)code_page, inst_list[i].opcodes, sizeof(uint32_t) * inst_list[i].count);
        code_page += PAGE_SIZE/4;
    }

    /* Configure the overlay values*/
    for (i = 0; i < NUM_PERM_INDEX; i++)
    {
        for (j = 0; j < PLANE_COUNT; j++)
        {
            cmd_ret = val_realm_rsi_mem_set_perm_value(j + 1, i + 1, overlay_val[i][j]);
            if (cmd_ret.x0) {
                LOG(ERROR, "MEM_SET_PERM_VALUE failed with : %d \n", cmd_ret.x0);
                LOG(TEST, "plane_idx  : %d, perm_idx : %d \n", j + 1, i + 1);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
                goto exit;
            }
        }
    }

    base = ipa_base ;
    top = ipa_base + PAGE_SIZE;

    /* Write the permission overlay index for the respective code and data granules */
    for (i = 0; i < TEST_PAGE_COUNT; i++)
    {
        while (base != top) {
            cmd_ret = val_realm_rsi_mem_set_perm_index(base, top, PermList[i], cookie_value);
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

        top  += PAGE_SIZE;
    }

    /* Run Plane with test stimulus defined in the test header file*/
    for (i = 0; i < (sizeof(test_data) / sizeof(test_stimulus)); i++)
    {
        for (j = 0; j < PLANE_COUNT; j++)
        {
            run_ptr.enter.pc = ipa_base + test_data[i].entry_pc;
            run_ptr.enter.gprs[0] = ipa_base + test_data[i].entry_x0;

            if (val_realm_run_plane(j + 1, &run_ptr))
            {
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
                goto exit;
            }

            esr = run_ptr.exit.esr_el2;

            if (run_ptr.exit.reason != RSI_EXIT_SYNC      ||
                ESR_EL2_EC(esr) != test_data[i].esr_el2_ec[j])
            {
                LOG(ERROR, "Invalid exit type: %d, ESR: 0x%lx\n",
                                             run_ptr.exit.reason, run_ptr.exit.esr_el2);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
                goto exit;
            }
        }
    }

exit:
    val_realm_return_to_host();
}

