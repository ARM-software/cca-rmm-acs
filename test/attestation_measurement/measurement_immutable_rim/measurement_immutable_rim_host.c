/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_command.h"
#define IPA_ADDR_UNASSIGNED 0x0
#define IPA_ADDR_UNASSIGNED1 0x1000

void measurement_immutable_rim_host(void)
{
    val_host_realm_ts realm;
    uint64_t ret;
    val_host_data_destroy_ts data_destroy;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm, 0))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    /* Add data granule */
    if (create_mapping(IPA_ADDR_UNASSIGNED, true, realm.rd))
    {
        LOG(ERROR, "\tCouldn't create the assigned protected mapping\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    uint64_t data = val_host_delegate_granule();
    uint64_t src = val_host_undelegate_granule();
    uint64_t flags = RMI_NO_MEASURE_CONTENT;

    if (val_host_rmi_data_create(realm.rd, data, IPA_ADDR_UNASSIGNED, src, flags))
    {
        LOG(ERROR, "\tCouldn't complete the assigned protected mapping\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    /* Activate the realm */
    ret = val_host_rmi_realm_activate(realm.rd);
    if (ret)
    {
        LOG(ERROR, "\tRealm activate failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    /* Remove the granule */
    ret = val_host_rmi_data_destroy(realm.rd, IPA_ADDR_UNASSIGNED, &data_destroy);
    if (ret)
    {
        LOG(ERROR, "\tData destroy failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    /* Add granule after activating realm */
    if (create_mapping(IPA_ADDR_UNASSIGNED1, false, realm.rd))
    {
        LOG(ERROR, "\tCouldn't create the assigned protected mapping\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    uint64_t data_unknown = val_host_delegate_granule();
    if (val_host_rmi_data_create_unknown(realm.rd, data_unknown, IPA_ADDR_UNASSIGNED1))
    {
        LOG(ERROR, "\tCouldn't complete the assigned protected mapping\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    }

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_realm:
    return;
}