/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"
#include "pal_arch_helpers.h"
#include "val_irq.h"
#include "val_realm_framework.h"

#define GIC_INTR_TIMEOUT 0x100000U

static volatile int gic_handler_flag;

static int gic_irq_handler(void)
{
    gic_handler_flag = 1;

    return 0;
}

static uint32_t gic_wait_for_irq(void)
{
    uint64_t timeout = GIC_INTR_TIMEOUT;

    while ((--timeout != 0U) && (gic_handler_flag == 0)) {
        ;
    }

    if (gic_handler_flag == 0) {
        LOG(ERROR, "Interrupt not triggered to realm\n");
        return VAL_ERROR;
    }

    gic_handler_flag = 0;
    return VAL_SUCCESS;
}

void gic_rec_exit_gic_state_preserved_realm(void)
{
    if (val_irq_register_handler(SPI_vINTID, gic_irq_handler)) {
        LOG(ERROR, "Interrupt %d register failed\n", SPI_vINTID);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    val_realm_return_to_host();

    if (gic_wait_for_irq() != VAL_SUCCESS) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    val_realm_return_to_host();

    write_icv_eoir1_el1(SPI_vINTID);
    write_icv_dir_el1(SPI_vINTID);
    val_realm_return_to_host();

exit:
    if (val_irq_unregister_handler(SPI_vINTID)) {
        LOG(ERROR, "Interrupt %d unregister failed\n", SPI_vINTID);
    }

    val_realm_return_to_host();
}
