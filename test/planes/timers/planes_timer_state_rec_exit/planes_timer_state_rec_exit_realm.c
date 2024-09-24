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
#include "val_timer.h"
#include "planes_timer_state_rec_exit_data.h"

static void p0_payload(void)
{
    uint64_t p1_ipa_base, p1_ipa_top;
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_plane_run_ts run_ptr = {0};
    val_realm_plane_enter_flags_ts plane_flags;
    uint64_t timer_cval, timer_freq, sys_count;
    uint64_t esr, i;

    val_memset(&plane_flags, 0, sizeof(plane_flags));

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
    plane_flags.gic_owner = RSI_GIC_OWNER_0;
    plane_flags.trap_hc = RSI_NO_TRAP;

    val_memcpy(&run_ptr.enter.flags, &plane_flags, sizeof(run_ptr.enter.flags));

    if (val_realm_run_plane(PLANE_1_INDEX, &run_ptr))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(test_stimulus)); i++)
    {
        esr = run_ptr.exit.esr_el2;

        /* Check that Plane exit was due to P0 HVC call beacauese of P1 requesting P0 cntp_cval */
        if (run_ptr.exit.reason != RSI_EXIT_SYNC ||
            ESR_EL2_EC(esr) != ESR_EL2_EC_HVC ||
            run_ptr.exit.gprs[0] != PSI_P0_CALL)
        {
            LOG(ERROR, "Invalid exit type: %d, ESR: 0x%lx",
                                                 run_ptr.exit.reason, run_ptr.exit.esr_el2)
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }

        sys_count = syscounter_read();
        timer_freq = read_cntfrq_el0();

        /* LOAD cntp_cval register */
        timer_cval = sys_count + (test_data[i].p0_deadline * timer_freq) / 1000;
        write_cntp_cval_el0(timer_cval);

        /* Enable or Disable Timer based on test data */
        if (test_data[i].p0_enable)
            write_cntp_ctl_el0((read_cntp_ctl_el0() | ARM_ARCH_TIMER_ENABLE)
                                                         & ~(ARM_ARCH_TIMER_IMASK));
        else
            val_disable_phy_timer_el1();

        /* Return cntp_cval value to P1 */
        run_ptr.enter.gprs[0] = timer_cval;

        /* Enter plane */
        plane_flags.gic_owner = RSI_GIC_OWNER_0;
        plane_flags.trap_hc = RSI_NO_TRAP;

        val_memcpy(&run_ptr.enter.flags, &plane_flags, sizeof(run_ptr.enter.flags));

        if (val_realm_run_plane(PLANE_1_INDEX, &run_ptr))
        {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }
    }

    val_disable_phy_timer_el1();

exit:
    val_realm_return_to_host();
}

static void p1_payload(void)
{
    uint64_t sys_count, p0_cval, i;
    uint64_t timer_cval, timer_freq;
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_host_call_t gv_realm_host_call = {0};

    for (i = 0; i < (sizeof(test_data) / sizeof(test_stimulus)); i++)
    {
        /* Request P0 to enable timer and retun sys count read */
        p0_cval = val_hvc_call(PSI_P0_CALL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0).x0;

        sys_count = syscounter_read();
        timer_freq = read_cntfrq_el0();

        /* LOAD cntp_cval register */
        timer_cval = sys_count + (test_data[i].p1_deadline * timer_freq) / 1000;
        write_cntp_cval_el0(timer_cval);

        /* Enable or Disable Timer based on test data */
        if (test_data[i].p1_enable)
            write_cntp_ctl_el0((read_cntp_ctl_el0() | ARM_ARCH_TIMER_ENABLE)
                                                         & ~(ARM_ARCH_TIMER_IMASK));
        else
            write_cntp_ctl_el0(read_cntp_ctl_el0() & ~(0x03UL));

        /* Return set cntp_cval value for P0 and P1 */
        gv_realm_host_call.gprs[0] = p0_cval;
        gv_realm_host_call.gprs[1] = timer_cval;

        /* Trigger a REC exit */
        if (val_realm_rsi_host_call_struct((uint64_t)&gv_realm_host_call))
        {
            LOG(ERROR, "HOST_CALL_FAILED\n", 0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto exit;
        }
    }

    val_disable_phy_timer_el1();

exit:
    val_realm_return_to_p0();
}

void planes_timer_state_rec_exit_realm(void)
{
    if (val_realm_in_p0())
        p0_payload();
    else
        p1_payload();
}

