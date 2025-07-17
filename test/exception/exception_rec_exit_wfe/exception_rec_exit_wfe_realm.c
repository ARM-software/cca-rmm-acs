/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "exception_common_realm.h"

void exception_rec_exit_wfe_realm(void)
{
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
    uint64_t index = 0;
    int gGPRS[EXCEPTION_TEST_MAX_GPRS];
    int lGPRS[EXCEPTION_TEST_MAX_GPRS];
    uint64_t *wfe_trig = (uint64_t *)(val_get_shared_region_base() + VAL_TEST_USE1);
    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Execute WFE in a loop to avoid conditions where PE treating it as NOP */
    for (uint8_t i = 0; i < WFX_ITERATIONS; i++)
    {
        /* Keep the current gprs information before triggering the realm exit(WFE) */
        GET_GPRS_VALUES(gGPRS);

        dsbsy();

        /* trigger the WFE to realm to exit */
        __asm__("wfe");

        /* Upon rec enter again Read the gprs again */
        GET_GPRS_VALUES(lGPRS);

        dsbsy();

        if (*wfe_trig == true)
            break;
    }

    /* Fail the test if WFE is not triggered */
    if (*wfe_trig == false)
    {
            LOG(ERROR, "WFE not triggered\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto test_exit;
    }

    /* Compare the new gprs with earlier saved one */
    for (index = 0; index < EXCEPTION_TEST_MAX_GPRS; index++)
    {
        if (gGPRS[index] != lGPRS[index])
        {
            LOG(ERROR, "WFE check, the gprs value is corrupted, hence testcase failed \n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto test_exit;
        }
    }

    LOG(TEST, "REALM WFE Trigger checks are verified \n");

test_exit:
    val_realm_return_to_host();
}
