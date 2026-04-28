/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "exception_common_realm.h"

void exception_rec_exit_wfi_realm(void)
{
    /*
     * Capture x0-x30 in one asm block directly to memory.
     * This avoids using register-bound C variables like:
     *   register int x29 __asm("x29");
     * which Coverity reports as uninitialized scalar usage.
     */
    #define GET_GPRS_VALUES(gprs_array)                                      \
        do {                                                                 \
            __asm__ volatile(                                                \
                "str x0,  [%0, #0]\n\t"                                      \
                "str x1,  [%0, #8]\n\t"                                      \
                "str x2,  [%0, #16]\n\t"                                     \
                "str x3,  [%0, #24]\n\t"                                     \
                "str x4,  [%0, #32]\n\t"                                     \
                "str x5,  [%0, #40]\n\t"                                     \
                "str x6,  [%0, #48]\n\t"                                     \
                "str x7,  [%0, #56]\n\t"                                     \
                "str x8,  [%0, #64]\n\t"                                     \
                "str x9,  [%0, #72]\n\t"                                     \
                "str x10, [%0, #80]\n\t"                                     \
                "str x11, [%0, #88]\n\t"                                     \
                "str x12, [%0, #96]\n\t"                                     \
                "str x13, [%0, #104]\n\t"                                    \
                "str x14, [%0, #112]\n\t"                                    \
                "str x15, [%0, #120]\n\t"                                    \
                "str x16, [%0, #128]\n\t"                                    \
                "str x17, [%0, #136]\n\t"                                    \
                "str x18, [%0, #144]\n\t"                                    \
                "str x19, [%0, #152]\n\t"                                    \
                "str x20, [%0, #160]\n\t"                                    \
                "str x21, [%0, #168]\n\t"                                    \
                "str x22, [%0, #176]\n\t"                                    \
                "str x23, [%0, #184]\n\t"                                    \
                "str x24, [%0, #192]\n\t"                                    \
                "str x25, [%0, #200]\n\t"                                    \
                "str x26, [%0, #208]\n\t"                                    \
                "str x27, [%0, #216]\n\t"                                    \
                "str x28, [%0, #224]\n\t"                                    \
                "str x29, [%0, #232]\n\t"                                    \
                "str x30, [%0, #240]\n\t"                                    \
                :                                                            \
                : "r"(gprs_array)                                            \
                : "memory");                                                 \
        } while (0)

    uint64_t index = 0;
    uint64_t gGPRS[EXCEPTION_TEST_MAX_GPRS];
    uint64_t lGPRS[EXCEPTION_TEST_MAX_GPRS];
    uint64_t *wfi_trig = (uint64_t *)(val_get_shared_region_base() + VAL_TEST_USE1);

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Execute WFI in a loop to avoid conditions where PE treating it as NOP */
    for (uint8_t i = 0; i < WFX_ITERATIONS; i++)
    {
        /* Before triggering the rec exit (due to WFI) save the GPR values */
        GET_GPRS_VALUES(gGPRS);

        dsbsy();

        /* Trigger the WFI for rec exit */
        __asm__ volatile("wfi" ::: "memory");

        /* Upon rec enter again read the GPRs again */
        GET_GPRS_VALUES(lGPRS);

        dsbsy();

        if (*wfi_trig == true)
            break;
    }

    /* Fail the test if WFI is not triggered */
    if (*wfi_trig == false)
    {
        LOG(ERROR, "WFI not triggered\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto test_exit;
    }

    /* Compare the new GPRs with earlier saved ones */
    for (index = 0; index < EXCEPTION_TEST_MAX_GPRS; index++)
    {
        if (gGPRS[index] != lGPRS[index])
        {
            LOG(ERROR, "WFI check, the gprs value is corrupted, "
                       "hence testcase failed\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto test_exit;
        }
    }

    LOG(TEST, "REALM WFI Trigger checks are verified \n");

test_exit:
    val_realm_return_to_host();
}

