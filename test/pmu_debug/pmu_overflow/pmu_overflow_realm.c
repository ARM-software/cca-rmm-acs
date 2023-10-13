/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"
#include "val_pmu.h"
#include "val_irq.h"

static volatile int handler_flag;

static int rmi_pmu_handler(void)
{
    /* Clear PMU interrupt */
    write_pmintenclr_el1(read_pmintenset_el1());
    isb();

    /* Write EOI register */
    val_gic_end_of_intr(PMU_VIRQ);

    handler_flag = 1;
    return 0;
}

void pmu_overflow_realm(void)
{
    uint64_t timeout = 0x10000000000;
    uint64_t dfr0;

    /* Below code is executed for REC[0] only */
    LOG(TEST, "\tIn realm_create_realm REC[0], mpdir=%x\n", val_read_mpidr(), 0);

    dfr0 = read_id_aa64dfr0_el1();
    /* Check that PMU is supported */
    if ((VAL_EXTRACT_BITS(dfr0, 8, 11) == 0x0))
    {
        LOG(ERROR, "\tPMU not supported\n", 0, 0);
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto exit;
    }

    if (val_irq_register_handler(PMU_VIRQ, rmi_pmu_handler))
    {
        LOG(ERROR, "\tInterrupt %d register failed\n", PMU_VIRQ, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    pmu_reset();

    write_pmevcntrn_el0(0, (uint64_t)PRE_OVERFLOW);
    enable_event_counter(0);

    /* Enable interrupt on event counter #0 */
    write_pmintenset_el1((1UL << 0));

    enable_counting();
    while ((read_pmintenset_el1() != 0UL) && --timeout)
    {
    }

    timeout = 0x1000000;
    while (--timeout && !handler_flag);
    if (handler_flag == 1)
    {
        handler_flag = 0;
    } else {
        LOG(ERROR, "\tPMU interrupt %d not triggered to realm\n", PMU_VIRQ, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
    }

exit:
    pmu_reset();

    if (val_irq_unregister_handler(PMU_VIRQ))
    {
        LOG(ERROR, "\tInterrupt %d unregister failed\n", PMU_VIRQ, 0);
    }

    val_realm_return_to_host();
}
