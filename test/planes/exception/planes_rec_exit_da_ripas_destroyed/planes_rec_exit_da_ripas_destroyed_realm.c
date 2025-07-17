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
    uint64_t p1_ipa_base, p1_ipa_top;
    uint64_t esr;
    uint64_t ipa_base, size;
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_plane_run_ts run_ptr = {0};

    /* Request Host the test IPA whose RIPAS is DESTROYED via host call*/
    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    ipa_base = gv_realm_host_call->gprs[1];
    size = gv_realm_host_call->gprs[2];

    /* Configure Permissions for Plane 1 image */
    p1_ipa_base = VAL_PLANE1_IMAGE_BASE_IPA;
    p1_ipa_top = p1_ipa_base + PLATFORM_REALM_IMAGE_SIZE;

    if (val_realm_plane_perm_init(PLANE_1_INDEX, PLANE_1_PERMISSION_INDEX, p1_ipa_base,
                                                                             p1_ipa_top))
    {
        LOG(ERROR, "Secondary plane permission initialization failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    /* Give Plane 1 RW permission for the test IPA */
    if (val_realm_plane_perm_init(PLANE_1_INDEX, TEST_IPA_PERM_INDEX, ipa_base, ipa_base + size))
    {
        LOG(ERROR, "Secondary plane permission initialization failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    /* Run Plane */
    run_ptr.enter.pc =  VAL_PLANE1_IMAGE_BASE_IPA;

    if (val_realm_run_plane(PLANE_1_INDEX, &run_ptr))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    esr = run_ptr.exit.esr_el2;

    /* Check that Plane exit was due to P0 HVC call beacauese of P1 requesting test IPA */
    if (run_ptr.exit.reason != RSI_EXIT_SYNC ||
        ESR_EL2_EC(esr) != ESR_EL2_EC_HVC ||
        run_ptr.exit.gprs[0] != PSI_P0_CALL)
    {
        LOG(ERROR, "Invalid exit type: %d, ESR: 0x%lx\n",
                                             run_ptr.exit.reason, run_ptr.exit.esr_el2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    /* Return the test IPA to P1 */
    run_ptr.enter.gprs[0] = ipa_base;

    /* Run Plane */
    if (val_realm_run_plane(PLANE_1_INDEX, &run_ptr))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}

static void p1_payload(void)
{
    val_memory_region_descriptor_ts mem_desc;
    uint64_t test_ipa;

    /* Request test IPA from P0 */
    test_ipa = val_hvc_call(PSI_P0_CALL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0).x0;

    /* Map the IPA as DATA in stage 1 */
    mem_desc.virtual_address = test_ipa;
    mem_desc.physical_address = test_ipa;
    mem_desc.length = PAGE_SIZE;
    mem_desc.attributes = MT_RW_DATA | MT_REALM;
    if (val_realm_pgt_create(&mem_desc))
    {
        LOG(ERROR, "VA to PA mapping failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    /* Test Intent: Protected IPA, RIPAS = DESTROYED
     * data access => REC exit to host */
    *(volatile uint32_t *)test_ipa = 0x100;


exit:
    val_realm_return_to_p0();
}

void planes_rec_exit_da_ripas_destroyed_realm(void)
{
    if (val_realm_in_p0())
        p0_payload();
    else
        p1_payload();
}

