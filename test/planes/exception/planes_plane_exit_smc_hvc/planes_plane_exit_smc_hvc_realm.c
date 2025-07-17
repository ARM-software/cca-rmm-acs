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
#include "val_hvc.h"

static void p0_payload(void)
{
    uint64_t p1_ipa_base, p1_ipa_top;

    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_plane_run_ts run_ptr = {0};

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

    if (val_realm_run_plane(PLANE_1_INDEX, &run_ptr))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    if (run_ptr.exit.reason != RSI_EXIT_SYNC ||
                                         ESR_EL2_EC(run_ptr.exit.esr_el2) != ESR_EL2_EC_SMC)
    {
        LOG(ERROR, "Invalid exit type: %d, ESR: 0x%lx\n",
                                             run_ptr.exit.reason, run_ptr.exit.esr_el2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    if (val_realm_run_plane(PLANE_1_INDEX, &run_ptr))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    if (run_ptr.exit.reason != RSI_EXIT_SYNC ||
                                         ESR_EL2_EC(run_ptr.exit.esr_el2) != ESR_EL2_EC_HVC)
    {
        LOG(ERROR, "Invalid exit type: %d, ESR: 0x%lx\n",
                                             run_ptr.exit.reason, run_ptr.exit.esr_el2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }


exit:
    val_realm_return_to_host();
}

static uint64_t p1_payload(void)
{
    /* Execute SMC */
    val_smc_call(PSI_TEST_SMC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    /* Execute HVC */
    val_hvc_call(PSI_TEST_HVC, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    return VAL_SUCCESS;
}

void planes_plane_exit_smc_hvc_realm(void)
{
    if (val_realm_in_p0())
        p0_payload();
    else
        p1_payload();
}

