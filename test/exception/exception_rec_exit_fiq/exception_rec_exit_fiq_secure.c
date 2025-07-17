/*
 * Copyright (c) 2023, 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_secure_framework.h"
#include "val_irq.h"

static volatile int handler_flag;

static int wd_irq_handler(void)
{
    val_twdog_disable();
    handler_flag = 1;

    return 0;
}

void exception_rec_exit_fiq_secure(void)
{

    if (val_irq_register_handler(PLATFORM_TWDOG_INTID, wd_irq_handler))
    {
        LOG(ERROR, "Secure WD interrupt register failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    val_irq_enable(PLATFORM_TWDOG_INTID, 0);
    val_configure_secure_irq_enable(PLATFORM_TWDOG_INTID);

    val_twdog_enable(100);
    val_secure_return_to_host();

    if (handler_flag == 1)
    {
        LOG(ALWAYS, "Secure WD Interrupt triggered\n");
    } else {
        LOG(ERROR, "Secure WD Interrupt not triggered\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
    }

    val_irq_disable(PLATFORM_TWDOG_INTID);

    if (val_irq_unregister_handler(PLATFORM_TWDOG_INTID))
    {
        LOG(ERROR, "  IRQ handler unregister failed\n");
    }

exit:
    if (handler_flag == 1)
        val_secure_return_to_preempted_host();
    else
        val_secure_return_to_host();
}
