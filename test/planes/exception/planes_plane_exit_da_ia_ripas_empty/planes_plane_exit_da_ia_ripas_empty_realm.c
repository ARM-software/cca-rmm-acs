/*
 * Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_planes.h"
#include "val_realm_framework.h"
#include "val_realm_memory.h"
#include "val_hvc.h"

#define TEST_IPA_PERM_INDEX     3

static void p0_payload(void)
{
    val_realm_rsi_host_call_t *gv_realm_host_call;
    val_smc_param_ts args;
    uint64_t p1_ipa_base, p1_ipa_top;
    uint64_t esr;
    uint64_t ipa_base, size;
    uint8_t ripas_val;
    uint64_t flags = RSI_NO_CHANGE_DESTROYED;
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_plane_run_ts run_ptr = {0};

    /* Request Host IPA whose HIPAS,RIPAS is ASSIGNED,RAM via host call*/
    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    ipa_base = gv_realm_host_call->gprs[1];
    size = gv_realm_host_call->gprs[2];
    ripas_val = RSI_EMPTY;

    /* Request Host to transition RIPAS from RAM to EMPTY */
    val_memset(&args, 0x0, sizeof(val_smc_param_ts));
    args = val_realm_rsi_ipa_state_set(ipa_base, ipa_base + size, ripas_val, flags);
    if (args.x0 || (args.x1 != (ipa_base + size)))
    {
        LOG(ERROR, "rsi_ipa_state_set failed x0 %lx x1 %lx\n", args.x0, args.x1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    /* Configure Permissions for Plane 1 image */
    p1_ipa_base = VAL_PLANE1_IMAGE_BASE_IPA;
    p1_ipa_top = p1_ipa_base + PLATFORM_REALM_IMAGE_SIZE;

    if (val_realm_plane_perm_init(PLANE_1_INDEX, PLANE_1_PERMISSION_INDEX, p1_ipa_base,
                                                                             p1_ipa_top))
    {
        LOG(ERROR, "Secondary plane permission initialization failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    /* Give Plane 1 RW permission for the test IPA */
    if (val_realm_plane_perm_init(PLANE_1_INDEX, TEST_IPA_PERM_INDEX, ipa_base, ipa_base + size))
    {
        LOG(ERROR, "Secondary plane permission initialization failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    /* Run Plane */
    run_ptr.enter.pc =  VAL_PLANE1_IMAGE_BASE_IPA;

    if (val_realm_run_plane(PLANE_1_INDEX, &run_ptr))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    esr = run_ptr.exit.esr_el2;

    /* Check that Plane exit was due to P0 HVC call beacauese of P1 requesting RIPAS EMPTY IPA */
    if (run_ptr.exit.reason != RSI_EXIT_SYNC ||
        ESR_EL2_EC(esr) != ESR_EL2_EC_HVC ||
        run_ptr.exit.gprs[0] != PSI_P0_CALL)
    {
        LOG(ERROR, "Invalid exit type: %d, ESR: 0x%lx\n",
                                             run_ptr.exit.reason, run_ptr.exit.esr_el2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    /* Return the RIPAS EMPTY IPA to P1 */
    run_ptr.enter.gprs[0] = ipa_base;

    /* Run Plane */
    if (val_realm_run_plane(PLANE_1_INDEX, &run_ptr))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    esr = run_ptr.exit.esr_el2;

    /* Check that Plane exit was due to P1 data access to RIPAS EMPTY IPA */
    if (run_ptr.exit.reason != RSI_EXIT_SYNC ||
        ESR_EL2_EC(esr) != ESR_EL2_EC_DATA_ABORT  ||
        run_ptr.exit.far_el2 != ipa_base)
    {
        LOG(ERROR, "Invalid exit type: %d, ESR: 0x%lx\n",
                                             run_ptr.exit.reason, run_ptr.exit.esr_el2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    /* Increment PC */
    run_ptr.enter.pc += 4;

    /* Run Plane */
    if (val_realm_run_plane(PLANE_1_INDEX, &run_ptr))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    esr = run_ptr.exit.esr_el2;

    /* Check that Plane exit was due to P1 instruction fetch from RIPAS EMPTY IPA */
    if (run_ptr.exit.reason != RSI_EXIT_SYNC ||
        ESR_EL2_EC(esr) != ESR_EL2_EC_INST_ABORT  ||
        run_ptr.exit.far_el2 != ipa_base)
    {
        LOG(ERROR, "Invalid exit type: %d, ESR: 0x%lx\n",
                                             run_ptr.exit.reason, run_ptr.exit.esr_el2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}

static void p1_payload(void)
{
    val_memory_region_descriptor_ts mem_desc;
    uint64_t empty_ipa;
    void (*fun_ptr)(void);

    /* Request unprotected IPA from P0 */
    empty_ipa = val_hvc_call(PSI_P0_CALL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0).x0;

    /* Map the IPA as DATA in stage 1 */
    mem_desc.virtual_address = empty_ipa;
    mem_desc.physical_address = empty_ipa;
    mem_desc.length = PAGE_SIZE;
    mem_desc.attributes = MT_RW_DATA | MT_REALM;
    if (val_realm_pgt_create(&mem_desc))
    {
        LOG(ERROR, "VA to PA mapping failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto exit;
    }

    fun_ptr = (void *)empty_ipa;
    /* Test Intent: Protected IPA, RIPAS=EMPTY data access => Plane exit to P0 */
    *(volatile uint32_t *)empty_ipa = 0x100;

    /* Re-map the IPA as CODE in stage 1 */
    mem_desc.attributes = MT_CODE | MT_REALM ;

    if (val_realm_update_attributes(PAGE_SIZE, empty_ipa, (uint32_t)mem_desc.attributes)) {
        LOG(ERROR, "Page attributes update failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto exit;
    }

    fun_ptr = (void *)empty_ipa;
    /* Test Intent: Protected IPA, RIPAS=EMPTY insrtuction access => Plane exit to P0 */
    (*fun_ptr)();


exit:
    val_realm_return_to_p0();
}

void planes_plane_exit_da_ia_ripas_empty_realm(void)
{
    if (val_realm_in_p0())
        p0_payload();
    else
        p1_payload();
}

