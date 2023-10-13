/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_exceptions.h"

static volatile uint64_t protected_phys;
static volatile uint64_t g_handler_abort;

static bool exception_gpf_handler(void)
{
    uint64_t esr_el1 = val_esr_el1_read();
    uint64_t far_el1 = val_far_el1_read();
    uint64_t next_pc = val_elr_el1_read() + 4;
    uint64_t ec = esr_el1 >> 26;

    if (ec != EC_DATA_ABORT_SAME_EL  || far_el1 != protected_phys)
    {
        LOG(ERROR, "\tUnexpected exception detected ec=%x, far=%x\n", ec, far_el1);
    } else
    {
        LOG(INFO, "\tExpected exception detected\n", 0, 0);
        g_handler_abort = 1;
    }

    /* Skip instruction that triggered the exception. */
    val_elr_el1_write(next_pc);

    /* Indicate that elr_el1 should not be restored. */
    return true;
}

void mm_gpf_exception_host(void)
{
#if (PLATFORM_GPF_SUPPORT_NS_EL2 == 1)
    val_host_realm_ts realm;

    val_memset(&realm, 0, sizeof(realm));

    val_host_realm_params(&realm);

    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm, true))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    val_exception_setup(NULL, exception_gpf_handler);
    protected_phys = PLATFORM_REALM_IMAGE_BASE;
    /* Test intent: Protected IPA, data access from Host => GPF exception taken to EL3/NS-EL2 */
    *(volatile uint32_t *)protected_phys = 0xabcd;
    if (!g_handler_abort)
    {
        LOG(ERROR, "\tGPF exception not triggered\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto free_exception;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
free_exception:
    val_exception_setup(NULL, NULL);
destroy_realm:
    return;
#else
    LOG(TEST, "\tPlatform not supporting GPF exception handling at NS-EL2\n", 0, 0);
    val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));

    (void)exception_gpf_handler;
    return;
#endif
}
