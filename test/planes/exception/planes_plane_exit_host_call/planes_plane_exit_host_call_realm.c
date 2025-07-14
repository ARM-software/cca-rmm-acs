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
#include "val_exceptions.h"

static void p0_payload(void)
{
    uint64_t p1_ipa_base, p1_ipa_top;
    uint64_t esr;
    val_realm_plane_enter_flags_ts plane_flags;

    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_plane_run_ts run_ptr = {0};

    val_memset(&plane_flags, 0, sizeof(plane_flags));

    p1_ipa_base = VAL_PLANE1_IMAGE_BASE_IPA;
    p1_ipa_top = p1_ipa_base + PLATFORM_REALM_IMAGE_SIZE;

    if (val_realm_plane_perm_init(PLANE_1_INDEX, PLANE_1_PERMISSION_INDEX, p1_ipa_base,
                                                                             p1_ipa_top))
    {
        LOG(ERROR, "Secondary plane permission initialization failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    run_ptr.enter.pc =  VAL_PLANE1_IMAGE_BASE_IPA;
    plane_flags.trap_hc = RSI_TRAP;

    val_memcpy(&run_ptr.enter.flags, &plane_flags, sizeof(run_ptr.enter.flags));

    if (val_realm_run_plane(PLANE_1_INDEX, &run_ptr))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    esr = run_ptr.exit.esr_el2;

    /* Check that Plane exit was due to RSI_HOST_CALL */
    if (run_ptr.exit.reason != RSI_EXIT_SYNC ||
        ESR_EL2_EC(esr) != ESR_EL2_EC_SMC ||
        run_ptr.exit.gprs[0] != RSI_HOST_CALL)
    {
        LOG(ERROR, "Invalid exit type: %d, ESR: 0x%lx\n",
                                             run_ptr.exit.reason, run_ptr.exit.esr_el2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}

static void p1_payload(void)
{
    /* Execute RSI_HOST_CALL */
    val_realm_rsi_host_call(VAL_SWITCH_TO_HOST);

    LOG(ERROR, "HOST CALL was not intercepted, Exiting Plane\n");
    val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
    val_realm_return_to_p0();
}

void planes_plane_exit_host_call_realm(void)
{
    if (val_realm_in_p0())
        p0_payload();
    else
        p1_payload();
}

