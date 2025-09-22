/*
 * Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
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

extern sea_params_ts g_sea_params;

void mm_ripas_destroyed_ia_realm(void)
{
    val_realm_rsi_host_call_t *gv_realm_host_call;
    val_memory_region_descriptor_ts mem_desc;
    void (*fun_ptr)(void);
    uint64_t ipa_base, size;

    /* Below code is executed for REC[0] only */
    LOG(DBG, "In realm_create_realm REC[0], mpdir=%x\n", val_read_mpidr());

    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    ipa_base = gv_realm_host_call->gprs[1];
    size = gv_realm_host_call->gprs[2];

    val_exception_setup(NULL, synchronous_exception_handler);

    /* Validate that host supplied IPA and size fall within the Realm protected region
     * and describe an aligned RIPAS=DESTROYED mapping. This prevents malicious host
     * values from being mapped as executable memory inside the Realm.
     */
    if ((size == 0) || (size % PAGE_SIZE))
    {
        LOG(ERROR, "Invalid RIPAS size: 0x%lx\n", size);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    uint64_t ipa_width = val_realm_get_ipa_width();
    if (ipa_width == 0)
    {
        LOG(ERROR, "Invalid IPA width\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    uint64_t protected_limit = 1ULL << (ipa_width - 1);
    uint64_t ipa_end = ipa_base + size;
    if ((ipa_base >= protected_limit) || (ipa_end > protected_limit) || (ipa_end < ipa_base) ||
        (ipa_base % PAGE_SIZE))
    {
        LOG(ERROR, "Invalid RIPAS range: base=0x%lx size=0x%lx\n", ipa_base, size);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    val_smc_param_ts ripas_state;
    val_memset(&ripas_state, 0, sizeof(ripas_state));
    ripas_state = val_realm_rsi_ipa_state_get(ipa_base, ipa_end);
    if ((ripas_state.x0 != RSI_SUCCESS) || (ripas_state.x2 != RSI_DESTROYED) ||
        (ripas_state.x1 < ipa_end))
    {
        LOG(ERROR, "Unexpected RIPAS state: x0=%lx x1=%lx x2=%lx\n",
            ripas_state.x0, ripas_state.x1, ripas_state.x2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    mem_desc.virtual_address = ipa_base;
    mem_desc.physical_address = ipa_base;
    mem_desc.length = size;
    mem_desc.attributes = MT_CODE | MT_REALM ;
    if (val_realm_pgt_create(&mem_desc))
    {
        LOG(ERROR, "VA to PA mapping failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    val_memset(&g_sea_params, 0, sizeof(g_sea_params));
    g_sea_params.abort_type = EC_INSTRUCTION_ABORT_SAME_EL;
    g_sea_params.far = ipa_base;
    /* Test intent: Protected IPA, RIPAS=DESTROYED instruction access
     * => REC exit due to instruction abort.
     */
    fun_ptr = (void *)ipa_base;
    (*fun_ptr)();
    if (g_sea_params.handler_abort)
    {
        LOG(ERROR, "Instruction abort triggered to Realm\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
    }



exit:
    val_exception_setup(NULL, NULL);
    val_realm_return_to_host();
}
