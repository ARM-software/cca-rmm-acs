/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_psci.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"
#include "val_realm_memory.h"

#define CONTEXT_ID 0x5555

void cmd_rec_enter_realm(void)
{
    uint64_t ret;
    val_realm_rsi_host_call_t *gv_realm_host_call;
    uint64_t test_ipa;
    val_memory_region_descriptor_ts mem_desc;

    if (val_get_primary_mpidr() != val_read_mpidr())
    {
        /* Execute PSCI sytem off which should change the realm(rd).state to SYSTEM_OFF */
        val_psci_system_off();
    }

    /* Request Host the test IPA whose HIPAS,RIPAS is UNASSIGNED,RAM via host call*/
    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    test_ipa = gv_realm_host_call->gprs[1];

    /* Map the IPA as DATA in stage 1 */
    mem_desc.virtual_address = test_ipa;
    mem_desc.physical_address = test_ipa;
    mem_desc.length = PAGE_SIZE;
    mem_desc.attributes = MT_RW_DATA | MT_REALM;
    if (val_realm_pgt_create(&mem_desc))
    {
        LOG(ERROR, "VA to PA mapping failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    /* Test Intent: Generate a REC exit from Pn to test subsequent entry path */
    *(volatile uint32_t *)test_ipa = 0x100;

    /* Execute PSCI_CPU_ON */
    ret = val_psci_cpu_on(REC_NUM(1), val_realm_get_secondary_cpu_entry(), CONTEXT_ID);
    if (ret)
    {
        LOG(ERROR, "PSCI CPU ON failed with ret status : 0x%x \n", ret);
        goto exit;
    }

exit:
    val_realm_return_to_host();
}


