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

static void p0_payload(void)
{
    val_realm_rsi_host_call_t *gv_realm_host_call;
    uint64_t p1_ipa_base, p1_ipa_top, unprot_ipa;
    uint64_t esr;

    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_plane_run_ts run_ptr = {0};

    /* Request Host for S2 mapped unprotected IPA via host call*/
    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    unprot_ipa = gv_realm_host_call->gprs[1];

    /* Configure Permissions for Plane 1 image */
    p1_ipa_base = VAL_PLANE1_IMAGE_BASE_IPA;
    p1_ipa_top = p1_ipa_base + PLATFORM_REALM_IMAGE_SIZE;

    if (val_realm_plane_perm_init(PLANE_1_INDEX, PLANE_1_PERMISSION_INDEX, p1_ipa_base,
                                                                             p1_ipa_top))
    {
        LOG(ERROR, "Secondary plane permission initialization failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    /* Run Plane */
    run_ptr.enter.pc =  VAL_PLANE1_IMAGE_BASE_IPA;

    if (val_realm_run_plane(PLANE_1_INDEX, &run_ptr))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    esr = run_ptr.exit.esr_el2;

    /* Check that Plane exit was due to P0 HVC call beacauese of P1 requesting Unprotected IPA */
    if (run_ptr.exit.reason != RSI_EXIT_SYNC ||
        ESR_EL2_EC(esr) != ESR_EL2_EC_HVC ||
        run_ptr.exit.gprs[0] != PSI_P0_CALL)
    {
        LOG(ERROR, "Invalid exit type: %d, ESR: 0x%lx",
                                             run_ptr.exit.reason, run_ptr.exit.esr_el2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    /* Return the unprotected IPA to P1 */
    run_ptr.enter.gprs[0] = unprot_ipa;

    /* Run Plane */
    if (val_realm_run_plane(PLANE_1_INDEX, &run_ptr))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    esr = run_ptr.exit.esr_el2;

    /* Check that Plane exit was due to P1 instruction fetch from unprotected IPA */
    if (run_ptr.exit.reason != RSI_EXIT_SYNC ||
        ESR_EL2_EC(esr) != ESR_EL2_EC_INST_ABORT  ||
        run_ptr.exit.far_el2 != unprot_ipa)
    {
        LOG(ERROR, "Invalid exit type: %d, ESR: 0x%lx",
                                             run_ptr.exit.reason, run_ptr.exit.esr_el2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}

static void p1_payload(void)
{
    val_memory_region_descriptor_ts mem_desc;
    uint64_t unprot_ipa;
    void (*fun_ptr)(void);

    /* Request unprotected IPA from P0 */
    unprot_ipa = val_hvc_call(PSI_P0_CALL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0).x0;

    /* Map the IPA as DATA in stage 1 */
    mem_desc.virtual_address = unprot_ipa;
    mem_desc.physical_address = unprot_ipa;
    mem_desc.length = PAGE_SIZE;
    mem_desc.attributes = MT_RW_DATA | MT_REALM;
    if (val_realm_pgt_create(&mem_desc))
    {
        LOG(ERROR, "\tVA to PA mapping failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    /* S2AP for Unprotected IPA is controlled by host, except that RMM ensures that
     * execute permission is never granted */
    /* Test Intent : Read/Write to the unprotected IPA should succeed */
    *(volatile uint32_t *)unprot_ipa = 0x100;

    /* Re-Map IPA as CODE */
    mem_desc.attributes = MT_CODE | MT_REALM ;

    if (val_realm_update_attributes(PAGE_SIZE, unprot_ipa, (uint32_t)mem_desc.attributes)) {
        LOG(ERROR, "\tPage attributes update failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    fun_ptr = (void *)unprot_ipa;
    /* Test Intent: UnProtected IPA insrtuction access => Plane exit to P0 */
    (*fun_ptr)();

exit:
    val_realm_return_to_p0();
}

void planes_s2ap_unprotected_realm(void)
{
    if (val_realm_in_p0())
        p0_payload();
    else
        p1_payload();
}

