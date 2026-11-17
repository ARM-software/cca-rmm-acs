/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"

#define PROTECTED_IPA 0x800000

void mm_rsi_ripas_empty_protected_ipa_host(void)
{
    val_host_realm_ts realm;
    val_host_rec_enter_ts *rec_enter;
    val_host_rec_exit_ts *rec_exit;
    val_data_create_ts data_create;
    uint64_t ret;
    uint64_t phys;
    uint64_t out_top;
    uint64_t ripas_ipa = PROTECTED_IPA;
    uint64_t ripas_size = PAGE_SIZE;

    val_memset(&realm, 0, sizeof(realm));
    val_host_realm_params(&realm);

    if (val_host_realm_setup(&realm, false))
    {
        LOG(ERROR, "Realm setup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    data_create.size = PAGE_SIZE;
    phys = (uint64_t)val_host_mem_alloc(PAGE_SIZE, (2 * data_create.size));
    if (phys == 0)
    {
        LOG(ERROR, "val_host_mem_alloc failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return;
    }

    data_create.src_pa = phys;
    data_create.target_pa = phys + data_create.size;
    data_create.ipa = ripas_ipa;
    data_create.rtt_alignment = PAGE_SIZE;
    ret = val_host_map_protected_data_to_realm(&realm, &data_create);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "val_host_map_protected_data_to_realm failed, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        return;
    }

    if (val_host_realm_activate(&realm))
    {
        LOG(ERROR, "Realm activate failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        return;
    }

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "Initial REC enter failed, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        return;
    }

    if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "Initial REC exit is not HOST_CALL\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        return;
    }

    rec_enter = &(((val_host_rec_run_ts *)realm.run[0])->enter);
    rec_enter->gprs[1] = ripas_ipa;
    rec_enter->gprs[2] = ripas_size;

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "REC enter for RIPAS change failed, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        return;
    }

    if (val_host_check_realm_exit_ripas_change((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "REC exit is not RIPAS_CHANGE\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        return;
    }

    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);
    if ((rec_exit->ripas_base != ripas_ipa) ||
        (rec_exit->ripas_top != (ripas_ipa + ripas_size)))
    {
        LOG(ERROR, "RIPAS change range mismatch: base=0x%lx top=0x%lx\n",
            rec_exit->ripas_base, rec_exit->ripas_top);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        return;
    }

    ret = val_host_rmi_rtt_set_ripas(realm.rd, realm.rec[0], rec_exit->ripas_base,
                                     rec_exit->ripas_top, &out_top);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_RTT_SET_RIPAS failed, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        return;
    }

    if (out_top != rec_exit->ripas_top)
    {
        LOG(ERROR, "RMI_RTT_SET_RIPAS did not complete range: out_top=0x%lx\n", out_top);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        return;
    }

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "REC enter for RSI access check failed, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        return;
    }

    if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "Final REC exit is not HOST_CALL\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
    }
}
