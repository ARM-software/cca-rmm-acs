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
#include "val_timer.h"
#include "val_hvc.h"

#define ENABLE_WFX_TRAP     0x3

static void p0_payload(void)
{
    uint64_t p1_ipa_base, p1_ipa_top;
    val_realm_rsi_host_call_t *gv_realm_host_call;
    uint64_t esr, timeout;

    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_plane_run_ts run_ptr = {0};

    /* Initailize and boot P1 */
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

    esr = run_ptr.exit.esr_el2;

    /* Check that Plane exit was due to P1 requesting for timeout via HVC call */
    if (run_ptr.exit.reason != RSI_EXIT_SYNC ||
        ESR_EL2_EC(esr) != ESR_EL2_EC_HVC ||
        run_ptr.exit.gprs[0] != PSI_P0_CALL)
    {
        LOG(ERROR, "Invalid exit type: %d, ESR: 0x%lx\n",
                                             run_ptr.exit.reason, run_ptr.exit.esr_el2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    /* Return to host after initialization and ask for timeout */
    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    timeout = gv_realm_host_call->gprs[1];

    /* Return timeout value to P1 */
    run_ptr.enter.gprs[0] = timeout;

    /* Resume P1 execution and wait for interrupt*/
    if (val_realm_run_plane(PLANE_1_INDEX, &run_ptr))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }


exit:
    val_realm_return_to_host();
}

static void p1_payload(void)
{
    uint64_t delay_ms, time;
    /* Return to P0 after booting P1 and ask for timeout */
    delay_ms = val_hvc_call(PSI_P0_CALL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0).x0;

    /* Wait for interrupt */
    time = val_sleep_elapsed_time(delay_ms);

    LOG(ERROR, " Timer interrupt not triggered %d\n", time);

    val_realm_return_to_p0();
}

void planes_rec_exit_irq_realm(void)
{
    if (val_realm_in_p0())
        p0_payload();
    else
        p1_payload();
}

