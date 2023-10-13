/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_realm_framework.h"
#include "val_irq.h"

#define INTR_TIMEOUT 0x100000

static volatile int handler_flag;

static int rmi_sgi_handler(void)
{
    handler_flag = 1;

    return 0;
}

static int rmi_spi_handler(void)
{
    handler_flag = 1;
    val_gic_end_of_intr(SPI_vINTID);

    return 0;
}

static int rmi_ppi_handler(void)
{
    handler_flag = 1;
    val_gic_end_of_intr(PPI_vINTID);

    return 0;
}

static uint32_t wait_for_interrupt(void)
{
    uint64_t timeout = INTR_TIMEOUT;

    while (--timeout && !handler_flag);
    if (handler_flag == 1)
    {
        handler_flag = 0;
    } else {
        LOG(ERROR, "\tInterrupt not triggered to realm \n", 0, 0);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void gic_ctrl_hcr_realm(void)
{

    if (val_irq_register_handler(SPI_vINTID, rmi_spi_handler))
    {
        LOG(ERROR, "\tInterrupt %d register failed\n", SPI_vINTID, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    if (val_irq_register_handler(PPI_vINTID, rmi_ppi_handler))
    {
        LOG(ERROR, "\tInterrupt %d register failed\n", PPI_vINTID, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    if (val_irq_register_handler(SGI_vINTID, rmi_sgi_handler))
    {
        LOG(ERROR, "\tInterrupt %d register failed\n", SGI_vINTID, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    val_realm_return_to_host();

    if (wait_for_interrupt())
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    val_realm_return_to_host();

    if (wait_for_interrupt())
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    val_realm_return_to_host();

    if (wait_for_interrupt())
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    val_gic_end_of_intr(SPI_vINTID + 1);

exit:
    if (val_irq_unregister_handler(SPI_vINTID))
    {
        LOG(ERROR, "\tInterrupt %d unregister failed\n", SPI_vINTID, 0);
    }
    if (val_irq_unregister_handler(PPI_vINTID))
    {
        LOG(ERROR, "\tInterrupt %d unregister failed\n", PPI_vINTID, 0);
    }
    if (val_irq_unregister_handler(SGI_vINTID))
    {
        LOG(ERROR, "\tInterrupt %d unregister failed\n", SGI_vINTID, 0);
    }

    val_realm_return_to_host();
}
