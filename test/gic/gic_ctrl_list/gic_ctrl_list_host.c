/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_irq.h"

static bool host_realm_handle_irq(val_host_realm_ts *realm, uint32_t irq, uint32_t state)
{
    val_host_rec_entry_ts *rec_entry = NULL;
    val_host_rec_exit_ts *rec_exit = NULL;
    uint64_t ret;

    rec_entry = &(((val_host_rec_run_ts *)realm->run[0])->entry);
    rec_exit = &(((val_host_rec_run_ts *)realm->run[0])->exit);

    /* Inject interrupt to realm using GIC list registers */
    rec_entry->gicv3_lrs[0] = ((uint64_t)state << GICV3_LR_STATE) | (0ULL << GICV3_LR_HW) |
                                (1ULL << GICV3_LR_GROUP) | irq;
    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm->rec[0], realm->run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return VAL_ERROR;
    }

    if (state == GICV3_LR_STATE_INACTIVE)
    {
        if (VAL_EXTRACT_BITS(rec_exit->gicv3_lrs[0], 62, 63) != GICV3_LR_STATE_INACTIVE)
        {
            LOG(ERROR, "\tGIC list reg interrupt state check failed, received %x expected %x\n",
                        VAL_EXTRACT_BITS(rec_exit->gicv3_lrs[0], 62, 63), GICV3_LR_STATE_INACTIVE);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            return VAL_ERROR;
        }
    } else {
         if (VAL_EXTRACT_BITS(rec_exit->gicv3_lrs[0], 62, 63) != GICV3_LR_STATE_ACTIVE)
        {
            LOG(ERROR, "\tGIC list reg interrupt state check failed, received %x expected %x\n",
                         VAL_EXTRACT_BITS(rec_exit->gicv3_lrs[0], 62, 63), GICV3_LR_STATE_ACTIVE);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            return VAL_ERROR;
        }

        ret = val_host_rmi_rec_enter(realm->rec[0], realm->run[0]);
        if (ret)
        {
            LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            return VAL_ERROR;
        }

        if (VAL_EXTRACT_BITS(rec_exit->gicv3_lrs[0], 62, 63) != GICV3_LR_STATE_INACTIVE)
        {
            LOG(ERROR, "\tGIC list reg interrupt state check failed, received %x expected %x\n",
                        VAL_EXTRACT_BITS(rec_exit->gicv3_lrs[0], 62, 63), GICV3_LR_STATE_INACTIVE);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            return VAL_ERROR;
        }
    }

    /* Reset the GIC list register */
    rec_entry->gicv3_lrs[0] = 0x0;
    return VAL_SUCCESS;
}

void gic_ctrl_list_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    /* Populate realm with one REC */
    if (val_host_realm_setup(&realm, true))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    if (host_realm_handle_irq(&realm, SPI_vINTID, GICV3_LR_STATE_PENDING))
        goto destroy_realm;

    if (host_realm_handle_irq(&realm, PPI_vINTID, GICV3_LR_STATE_PENDING))
        goto destroy_realm;

    if (host_realm_handle_irq(&realm, SGI_vINTID, GICV3_LR_STATE_PENDING))
        goto destroy_realm;

    if (host_realm_handle_irq(&realm, SPI_vINTID, GICV3_LR_STATE_INACTIVE))
        goto destroy_realm;

    if (host_realm_handle_irq(&realm, PPI_vINTID, GICV3_LR_STATE_INACTIVE))
        goto destroy_realm;

    if (host_realm_handle_irq(&realm, SGI_vINTID, GICV3_LR_STATE_INACTIVE))
        goto destroy_realm;

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
