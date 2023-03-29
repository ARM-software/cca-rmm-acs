 /*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "exception_common_host.h"
#include "val_host_rmi.h"

#define MAP_LEVEL_FOR_WALK 3
#define EXCEPTION_IA_IPA_ADDRESS 0x501000

void exception_rec_exit_ia_host(void)
{
    val_host_realm_ts realm = {0,};
    uint64_t ret = 0;
    val_host_rec_entry_ts *rec_entry = NULL;

    val_host_rmifeatureregister0_ts featureregister0 = {0,};
    exception_rec_exit_ts exception_rec_exit;

    featureregister0.s2sz = 40;
    featureregister0.hash_sha_256 = 1;
    val_memcpy(&realm.realm_feat_0, &featureregister0, sizeof(val_host_rmifeatureregister0_ts));
    realm.hash_algo = RMI_SHA256;
    realm.s2_starting_level = 0;
    realm.num_s2_sl_rtts = 1;
    realm.vmid = 0;
    realm.rec_count = 1;
    uint64_t faulting_addr = 0;

    /* Populate realm with one RECs*/
    if (val_host_realm_setup(&realm, 0))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    if (val_host_ripas_init(&realm, EXCEPTION_IA_IPA_ADDRESS, VAL_RTT_MAX_LEVEL, PAGE_SIZE))
    {
        LOG(ERROR, "\trealm_init_ipa_state failed, ipa=0x%x\n",
                faulting_addr, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto destroy_realm;
    }

    /* Activate realm */
    if (val_host_realm_activate(&realm))
    {
        LOG(ERROR, "\tRealm activate failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    rec_entry = &(((val_host_rec_run_ts *)realm.run[0])->entry);
    /* Enter the realm through rec enter*/
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    /* pass the address to realm */
    rec_entry->gprs[1] = EXCEPTION_IA_IPA_ADDRESS;

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    /* extract the ec and imm values from the esr */
    exception_rec_exit.realm = &realm;
    ret = exception_validate_rec_exit_ia(&exception_rec_exit);
    if (ret)
    {
        LOG(ERROR, "\texception_validate_rec_exit_ia failed, ipa=0x%x\n",
                EXCEPTION_IA_IPA_ADDRESS, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));
destroy_realm:
    return;
}
