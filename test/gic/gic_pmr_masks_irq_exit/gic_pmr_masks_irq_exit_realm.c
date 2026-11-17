/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"
#include "val_exceptions.h"
#include "val_realm_framework.h"

static void gic_pmr_masks_irq_exit_unexpected_irq(void)
{
    LOG(ERROR, "Masked interrupt was delivered to Realm\n");
    val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
}

void gic_pmr_masks_irq_exit_realm(void)
{
    val_exception_setup(gic_pmr_masks_irq_exit_unexpected_irq, NULL);
    val_realm_return_to_host();
    val_realm_return_to_host();
}
