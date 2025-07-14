/*
 * Copyright (c) 2023, 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_realm_framework.h"
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

    /* Exit realm without EOIR */
    val_realm_return_to_host();

    /* Write EOI register */
    val_gic_end_of_intr(irq);
    val_gic_end_of_intr(irq);

    val_realm_return_to_host();
    return VAL_SUCCESS;
}

void gic_ctrl_list_realm(void)
{

    /* Below code is executed for REC[0] only */
    LOG(TEST, "In realm_create_realm REC[0], mpdir=%x\n", val_read_mpidr());
    if (val_irq_register_handler(SPI_vINTID, rmi_irq_handler))
    {
        LOG(ERROR, "Interrupt %d register failed\n", SPI_vINTID);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    if (val_irq_register_handler(PPI_vINTID, rmi_irq_handler))
    {
        LOG(ERROR, "Interrupt %d register failed\n", PPI_vINTID);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    if (val_irq_register_handler(SGI_vINTID, rmi_irq_handler))
    {
        LOG(ERROR, "Interrupt %d register failed\n", SGI_vINTID);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    val_realm_return_to_host();
    if (gic_eoir(SPI_vINTID))
        goto exit;

    if (gic_eoir(PPI_vINTID))
        goto exit;

    if (gic_eoir(SGI_vINTID))
        goto exit;

    /* Exit realm without EOIR */
    val_realm_return_to_host();
    val_realm_return_to_host();
    val_realm_return_to_host();

exit:
    if (val_irq_unregister_handler(SPI_vINTID))
    {
        LOG(ERROR, "Interrupt %d unregister failed\n", SPI_vINTID);
    }
    if (val_irq_unregister_handler(PPI_vINTID))
    {
        LOG(ERROR, "Interrupt %d unregister failed\n", PPI_vINTID);
    }
    if (val_irq_unregister_handler(SGI_vINTID))
    {
        LOG(ERROR, "Interrupt %d unregister failed\n", SGI_vINTID);
    }

}
