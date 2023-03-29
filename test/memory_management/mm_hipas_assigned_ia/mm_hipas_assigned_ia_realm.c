/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_realm_framework.h"
#include "val_realm_rsi.h"
#include "val_exceptions.h"
#include "mm_common_realm.h"

static volatile uint64_t ipa_base;
extern sea_params_ts g_sea_params;

void mm_hipas_assigned_ia_realm(void)
{
    void (*fun_ptr)(void);

    /* Below code is executed for REC[0] only */
    LOG(DBG, "\tIn realm_create_realm REC[0], mpdir=%x\n", val_read_mpidr(), 0);

    val_realm_return_to_host();
    ipa_base = (uint64_t)val_get_shared_region_base();

    val_exception_setup(NULL, synchronous_exception_handler);

    val_memset(&g_sea_params, 0, sizeof(g_sea_params));
    g_sea_params.abort_type = EC_INSTRUCTION_ABORT_SAME_EL;
    g_sea_params.far = ipa_base;

    fun_ptr = (void *)ipa_base;
    /* Test Intent: UnProtected IPA, insrtuction access => SEA to Realm */
    (*fun_ptr)();

    LOG(ERROR, "\tInstruction abort not triggered to Realm\n", 0, 0);
    val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));

    val_exception_setup(NULL, NULL);
    val_realm_return_to_host();
}
