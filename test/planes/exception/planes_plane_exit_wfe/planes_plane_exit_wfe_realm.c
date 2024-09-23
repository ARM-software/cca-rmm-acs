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
#include "val_exceptions.h"

#define ENABLE_WFX_TRAP     0x3

static void p0_payload(void)
{
    uint64_t p1_ipa_base, p1_ipa_top;
    uint64_t esr;

    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_plane_run_ts run_ptr = {0};

    p1_ipa_base = VAL_PLANE1_IMAGE_BASE_IPA;
    p1_ipa_top = p1_ipa_base + PLATFORM_REALM_IMAGE_SIZE;

    if (val_realm_plane_perm_init(PLANE_1_INDEX, PLANE_1_PERMISSION_INDEX, p1_ipa_base,
                                                                             p1_ipa_top))
    {
        LOG(ERROR, "Secondary plane permission initialization failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    run_ptr.enter.pc =  VAL_PLANE1_IMAGE_BASE_IPA;
    run_ptr.enter.flags =  ENABLE_WFX_TRAP;

    if (val_realm_run_plane(PLANE_1_INDEX, &run_ptr))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    esr = run_ptr.exit.esr_el2;

    if (run_ptr.exit.reason != RSI_EXIT_SYNC ||
        ESR_EL2_EC(esr) != ESR_EL2_EC_WFX ||
        ESR_EL2_WFX_TI(esr) != ESR_EL2_WFX_TI_WFE)
    {
        LOG(ERROR, "Invalid exit type: %d, ESR: 0x%lx",
                                             run_ptr.exit.reason, run_ptr.exit.esr_el2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

exit:
    val_realm_return_to_host();
}

static void p1_payload(void)
{
    /* Execute WFE in a loop to avoid conditions where PE treating it as NOP */
    for (uint8_t i = 0; i < WFX_ITERATIONS; i++)
    {
        __asm__("wfe");
    }

    LOG(ERROR, "WFE was not triggered, Exiting Plane\n", 0, 0);
    val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
    val_realm_return_to_p0();
}

void planes_plane_exit_wfe_realm(void)
{
    if (val_realm_in_p0())
        p0_payload();
    else
        p1_payload();
}

