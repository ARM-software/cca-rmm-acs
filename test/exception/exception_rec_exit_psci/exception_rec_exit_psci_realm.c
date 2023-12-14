/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "exception_common_realm.h"

#define CONTEXT_ID 0x5555

void exception_rec_exit_psci_realm(void)
{
    uint64_t index = 0, affinity = 0, mpidr = 0, ret;
    int lGPRS[EXCEPTION_TEST_MAX_GPRS];
    int gGPRS[EXCEPTION_TEST_MAX_GPRS];

    register int x0 __asm("x0");
    register int x1 __asm("x1");
    register int x2 __asm("x2");
    register int x3 __asm("x3");
    register int x4 __asm("x4");
    register int x5 __asm("x5");
    register int x6 __asm("x6");
    register int x7 __asm("x7");
    register int x8 __asm("x8");
    register int x9 __asm("x9");
    register int x10 __asm("x10");
    register int x11 __asm("x11");
    register int x12 __asm("x12");
    register int x13 __asm("x13");
    register int x14 __asm("x14");
    register int x15 __asm("x15");
    register int x16 __asm("x16");
    register int x17 __asm("x17");
    register int x18 __asm("x18");
    register int x19 __asm("x19");
    register int x20 __asm("x20");
    register int x21 __asm("x21");
    register int x22 __asm("x22");
    register int x23 __asm("x23");
    register int x24 __asm("x24");
    register int x25 __asm("x25");
    register int x26 __asm("x26");
    register int x27 __asm("x27");
    register int x28 __asm("x28");
    register int x29 __asm("x29");
    register int x30 __asm("x30");

    #define GET_GPRS_VALUES(gprs_array) \
        do { \
            gprs_array[0] = x0; \
            gprs_array[0] = x0; \
            gprs_array[1] = x1; \
            gprs_array[2] = x2; \
            gprs_array[3] = x3; \
            gprs_array[4] = x4; \
            gprs_array[5] = x5; \
            gprs_array[6] = x6; \
            gprs_array[7] = x7; \
            gprs_array[8] = x8; \
            gprs_array[9] = x9; \
            gprs_array[10] = x10; \
            gprs_array[11] = x11; \
            gprs_array[12] = x12; \
            gprs_array[13] = x13; \
            gprs_array[14] = x14; \
            gprs_array[15] = x15; \
            gprs_array[16] = x16; \
            gprs_array[17] = x17; \
            gprs_array[18] = x18; \
            gprs_array[19] = x19; \
            gprs_array[20] = x20; \
            gprs_array[21] = x21; \
            gprs_array[22] = x22; \
            gprs_array[23] = x23; \
            gprs_array[24] = x24; \
            gprs_array[25] = x25; \
            gprs_array[26] = x26; \
            gprs_array[27] = x27; \
            gprs_array[28] = x28; \
            gprs_array[29] = x29; \
            gprs_array[30] = x30; \
        } while (0)

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Before triggering the rec exit(dueto hostcall) save the gprs values */
    GET_GPRS_VALUES(gGPRS);
    mpidr = 1;
    /* get the second rec affinity*/
    affinity = EXCEPTION_AFFINITY_FROM_MPIDR(mpidr);
    LOG(TEST, "\tAffinity info: mpidr : 0x%x, affinity : %d  \n",\
              mpidr, affinity);
    /* testing the rec exit dueto psci affinity info */
    val_psci_affinity_info(affinity, 0);

    /* Upon rec enter again Read the gprs again and compare with earlier saved one */
    GET_GPRS_VALUES(lGPRS);

    for (index = 0; index < EXCEPTION_TEST_PSCI_GPRS_CHECK_COUNT; index++)
    {
        if (gGPRS[index] != lGPRS[index])
        {
            LOG(TEST, "\tPSCI check, the gprs value is corrupted, \
                              hence testcase failed : line %d\n", __LINE__, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto test_exit;
        }
    }

    /* Check for host rejecting PSCI request */
    ret = val_psci_cpu_on(affinity, val_realm_get_secondary_cpu_entry(), CONTEXT_ID);
    if (ret != PSCI_E_DENIED)
    {
        LOG(ERROR, "\n\tInvalid PSCI return code : ret = 0x%x \n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto test_exit;
    }


    LOG(ALWAYS, "\tREALM PSCI Trigger checks are verified \n", 0, 0);

test_exit:
    val_realm_return_to_host();
}

