/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "pal.h"

#ifndef SECURE_TEST_ENABLE
static event_t cpu_booted[32];
static val_host_realm_ts realm;

static void secondary_cpu(void)
{

    uint64_t mpidr = val_read_mpidr() & PAL_MPIDR_AFFINITY_MASK, ret;

    LOG(ALWAYS, "\tSecondary cpu with mpidr 0x%x booted\n", mpidr, 0);

    /* Enter Realm-1 on secondary cpu */
    ret = val_host_rmi_rec_enter(realm.rec[1], realm.run[1]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
    }

    /* Check that REC exit was due to PSCI_CPU_OFF  */
    if (val_host_check_realm_exit_psci((val_host_rec_run_ts *)realm.run[1],
                                PSCI_CPU_OFF))
    {
        LOG(ERROR, "\tSomething went wrong\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
    }

    /* Tell the primary cpu that the calling cpu has completed the test */
    val_send_event(&cpu_booted[val_get_cpuid(mpidr)]);

    val_host_power_off_cpu();
}
#endif

void cmd_multithread_realm_mp_host(void)
{
#ifdef SECURE_TEST_ENABLE
    /* Secure infrasturcure does not support MP boot yet, hence skipping the test */
    val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
    goto destroy_realm;
#else
    uint64_t ret, primary_mpidr, mpidr;
    uint32_t i;

    if (val_get_primary_mpidr() != val_read_mpidr())
        secondary_cpu();

    /* Below code only be executed by primary cpu */

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    realm.rec_count = 2;

    /* Populate realm with two RECs*/
    if (val_host_realm_setup(&realm, 1))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    /* Check that REC exit was due to PSCI_CPU_ON  */
    if (val_host_check_realm_exit_psci((val_host_rec_run_ts *)realm.run[0],
                                PSCI_CPU_ON_AARCH64))
    {
        LOG(ERROR, "\tSomething went wrong\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    /* Complete pending PSCI  */
    ret = val_host_rmi_psci_complete(realm.rec[0], realm.rec[1]);
    if (ret)
    {
        LOG(ERROR, "\tval_rmi_psci_complete, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    /* Wake up the one of the secondary cpu to execute rec enter */
    primary_mpidr = val_read_mpidr() & PAL_MPIDR_AFFINITY_MASK;

    for (i = 0; i < val_get_cpu_count(); i++)
    {
        val_init_event(&cpu_booted[i]);

        mpidr = val_get_mpidr(i);

        if (mpidr == primary_mpidr)
        {
            continue;
        }

        LOG(DBG, "\tPower up secondary cpu mpidr=%x\n", mpidr, 0);
        ret = val_host_power_on_cpu(i);
        if (ret != 0)
        {
            LOG(ERROR, "\tval_power_on_cpu mpidr 0x%x returns %x\n", mpidr, ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
            goto destroy_realm;
        }

        val_wait_for_event(&cpu_booted[i]);
        LOG(DBG, "\tPowered off secondary cpu mpidr=%x\n", mpidr, 0);
        break;
    }

    /* Resume back REC[0] execution */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_realm;
    } else if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)realm.run[0]))
    {
        LOG(ERROR, "\tSomething went wrong\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
#endif
destroy_realm:
    return;
}
