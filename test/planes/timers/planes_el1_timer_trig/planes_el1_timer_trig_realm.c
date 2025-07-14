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
#include "val_timer.h"
#include "val_irq.h"

#define INTR_TIMEOUT 0x100000

static volatile int handler_flag;

static int rmi_irq_handler(void)
{
    handler_flag = 1;

    return 0;
}

static int gic_eoir(uint32_t irq)
{
    uint64_t timeout = INTR_TIMEOUT;

    while (--timeout && !handler_flag);
    if (handler_flag == 1)
    {
        handler_flag = 0;
    } else {
        LOG(ERROR, "Interrupt %d not triggered to realm\n", irq);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return VAL_ERROR;
    }

    /* Write EOI register */
    val_gic_end_of_intr(irq);

    return VAL_SUCCESS;
}

static void p0_payload(void)
{
    uint64_t p1_ipa_base, p1_ipa_top;
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_plane_run_ts run_ptr = {0};
    val_realm_plane_enter_flags_ts plane_flags;

    val_memset(&plane_flags, 0, sizeof(plane_flags));

    if (val_irq_register_handler(IRQ_PHY_TIMER_EL1, rmi_irq_handler))
    {
        LOG(ERROR, "Interrupt %d register failed\n", IRQ_PHY_TIMER_EL1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    /* Configure Permissions for Plane 1 image */
    p1_ipa_base = VAL_PLANE1_IMAGE_BASE_IPA;
    p1_ipa_top = p1_ipa_base + PLATFORM_REALM_IMAGE_SIZE;

    if (val_realm_plane_perm_init(PLANE_1_INDEX, PLANE_1_PERMISSION_INDEX, p1_ipa_base,
                                                                             p1_ipa_top))
    {
        LOG(ERROR, "Secondary plane permission initialization failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto free_irq;
    }

    /* Run Plane */
    run_ptr.enter.pc =  VAL_PLANE1_IMAGE_BASE_IPA;
    plane_flags.gic_owner = RSI_GIC_OWNER_0;

    if (val_realm_run_plane(PLANE_1_INDEX, &run_ptr))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto free_irq;
    }

    /* Check that Plane exit was due to timer interrupt*/
    if (run_ptr.exit.reason != RSI_EXIT_SYNC)
    {
        LOG(ERROR, "Invalid exit type: %d, ESR: 0x%lx\n",
                                             run_ptr.exit.reason, run_ptr.exit.esr_el2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto free_irq;
    }

    if (gic_eoir(IRQ_PHY_TIMER_EL1))
        goto free_irq;

    val_disable_phy_timer_el1();

    /* Inject virtual interrupt to Pn */
    run_ptr.enter.gicv3_lrs[0] = ((uint64_t)GICV3_LR_STATE_PENDING << GICV3_LR_STATE)
                  | (0ULL << GICV3_LR_HW) | (1ULL << GICV3_LR_GROUP) | IRQ_PHY_TIMER_EL1;

    plane_flags.gic_owner = RSI_GIC_OWNER_0;

    val_memcpy(&run_ptr.enter.flags, &plane_flags, sizeof(run_ptr.enter.flags));

    /* Run Plane */
    if (val_realm_run_plane(PLANE_1_INDEX, &run_ptr))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto free_irq;
    }

free_irq:
    if (val_irq_unregister_handler(IRQ_PHY_TIMER_EL1))
    {
        LOG(ERROR, "Interrupt %d unregister failed\n", IRQ_PHY_TIMER_EL1);
    }

exit:
    val_realm_return_to_host();
}

static void p1_payload(void)
{
    uint64_t time;

    if (val_irq_register_handler(IRQ_PHY_TIMER_EL1, rmi_irq_handler))

    {
        LOG(ERROR, "Interrupt %d register failed\n", IRQ_PHY_TIMER_EL1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    /* Set Timer to fire after 1ms */
    val_timer_set_phy_el1(1000000, false);

    /* Wait for 2ms */
    time =  val_sleep_elapsed_time(2);

    if (!handler_flag)
        LOG(ERROR, "Timer interrupt not triggered %d \n", time);

    /* Clear the Interrupt after returning bakc to Pn */
    if (gic_eoir(IRQ_PHY_TIMER_EL1))
        goto free_irq;

    val_disable_phy_timer_el1();

free_irq:
    if (val_irq_unregister_handler(IRQ_PHY_TIMER_EL1))
    {
        LOG(ERROR, "Interrupt %d unregister failed\n", IRQ_PHY_TIMER_EL1);
    }

exit:
    val_realm_return_to_p0();
}

void planes_el1_timer_trig_realm(void)
{
    if (val_realm_in_p0())
        p0_payload();
    else
        p1_payload();
}

