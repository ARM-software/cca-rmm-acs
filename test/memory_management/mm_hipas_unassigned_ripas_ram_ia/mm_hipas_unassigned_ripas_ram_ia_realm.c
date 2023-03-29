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
#include "mm_common_realm.h"

static volatile uint64_t ipa_base, size;
extern sea_params_ts g_sea_params;

void mm_hipas_unassigned_ripas_ram_ia_realm(void)
{
    uint8_t ripas_val;
    val_realm_rsi_host_call_t *gv_realm_host_call;
    val_smc_param_ts args;
    val_memory_region_descriptor_ts mem_desc;
    val_pgt_descriptor_ts pgt_desc;
    void (*fun_ptr)(void);

    /* Below code is executed for REC[0] only */
    LOG(DBG, "\tIn realm_create_realm REC[0], mpdir=%x\n", val_read_mpidr(), 0);

    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    ipa_base = gv_realm_host_call->gprs[1];
    size = gv_realm_host_call->gprs[2];
    ripas_val = RSI_EMPTY;
    val_memset(&args, 0x0, sizeof(val_smc_param_ts));
    args = val_realm_rsi_ipa_state_set(ipa_base, size, ripas_val);
    if (args.x0 || (args.x1 != (ipa_base + size)))
    {
        LOG(ERROR, "\trsi_ipa_state_set failed x0 %lx x1 %lx\n", args.x0, args.x1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    ripas_val = RSI_RAM;
    val_memset(&args, 0x0, sizeof(val_smc_param_ts));
    args = val_realm_rsi_ipa_state_set(ipa_base, size, ripas_val);
    if (args.x0 || (args.x1 != (ipa_base + size)))
    {
        LOG(ERROR, "\trsi_ipa_state_set failed x0 %lx x1 %lx\n", args.x0, args.x1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    val_exception_setup(NULL, synchronous_exception_handler);

    pgt_desc.ttbr = tt_l0_base;
    pgt_desc.stage = PGT_STAGE1;
    pgt_desc.ias = PGT_IAS;
    pgt_desc.oas = PAGT_OAS;

    mem_desc.virtual_address = ipa_base;
    mem_desc.physical_address = ipa_base;
    mem_desc.length = size;
    mem_desc.attributes = ATTR_RW_DATA | ATTR_NS;
    if (val_pgt_create(pgt_desc, &mem_desc))
    {
        LOG(ERROR, "\tVA to PA mapping failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    val_memset(&g_sea_params, 0, sizeof(g_sea_params));
    g_sea_params.abort_type = EC_INSTRUCTION_ABORT_SAME_EL;
    g_sea_params.far = ipa_base;
    /* Test intent: Protected IPA, HIPAS=UNASSIGNED, RIPAS=RAM instruction access
     * => REC exit due to instruction abort.
     */
    fun_ptr = (void *)ipa_base;
    (*fun_ptr)();

    LOG(ERROR, "\tInstruction abort not triggered\n", 0, 0);
    val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
exit:
    val_exception_setup(NULL, NULL);
    val_realm_return_to_host();
}
