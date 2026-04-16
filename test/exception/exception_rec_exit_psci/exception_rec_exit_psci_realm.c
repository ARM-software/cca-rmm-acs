/*
 * Copyright (c) 2023, 2025-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "exception_common_realm.h"

#define CONTEXT_ID 0x5555
#define EXCEPTION_STORE_ALL_GPRS(base_reg)                                    \
    "str x0,  [" base_reg ", #0]\n\t"                                         \
    "str x1,  [" base_reg ", #8]\n\t"                                         \
    "str x2,  [" base_reg ", #16]\n\t"                                        \
    "str x3,  [" base_reg ", #24]\n\t"                                        \
    "str x4,  [" base_reg ", #32]\n\t"                                        \
    "str x5,  [" base_reg ", #40]\n\t"                                        \
    "str x6,  [" base_reg ", #48]\n\t"                                        \
    "str x7,  [" base_reg ", #56]\n\t"                                        \
    "str x8,  [" base_reg ", #64]\n\t"                                        \
    "str x9,  [" base_reg ", #72]\n\t"                                        \
    "str x10, [" base_reg ", #80]\n\t"                                        \
    "str x11, [" base_reg ", #88]\n\t"                                        \
    "str x12, [" base_reg ", #96]\n\t"                                        \
    "str x13, [" base_reg ", #104]\n\t"                                       \
    "str x14, [" base_reg ", #112]\n\t"                                       \
    "str x15, [" base_reg ", #120]\n\t"                                       \
    "str x16, [" base_reg ", #128]\n\t"                                       \
    "str x17, [" base_reg ", #136]\n\t"                                       \
    "str x18, [" base_reg ", #144]\n\t"                                       \
    "str x19, [" base_reg ", #152]\n\t"                                       \
    "str x20, [" base_reg ", #160]\n\t"                                       \
    "str x21, [" base_reg ", #168]\n\t"                                       \
    "str x22, [" base_reg ", #176]\n\t"                                       \
    "str x23, [" base_reg ", #184]\n\t"                                       \
    "str x24, [" base_reg ", #192]\n\t"                                       \
    "str x25, [" base_reg ", #200]\n\t"                                       \
    "str x26, [" base_reg ", #208]\n\t"                                       \
    "str x27, [" base_reg ", #216]\n\t"                                       \
    "str x28, [" base_reg ", #224]\n\t"                                       \
    "str x29, [" base_reg ", #232]\n\t"                                       \
    "str x30, [" base_reg ", #240]\n\t"

static uint64_t exception_capture_psci_affinity_info(uint64_t affinity,
                                                     uint64_t *gGPRS,
                                                     uint64_t *lGPRS)
{
    uint64_t ret;

    /*
     * Capture x0-x30 immediately before the PSCI exit and immediately after
     * resume in one asm block so the compiler cannot insert C code between
     * the SMC and the second snapshot.
     */
    __asm__ volatile(
        EXCEPTION_STORE_ALL_GPRS("%x[before]")
        "mov x0, %x[fid]\n\t"
        "mov x1, %x[target_affinity]\n\t"
        "mov x2, xzr\n\t"
        "mov x3, xzr\n\t"
        "smc #0\n\t"
        EXCEPTION_STORE_ALL_GPRS("%x[after]")
        "mov %x[result], x0\n\t"
        : [result] "=&r" (ret)
        : [before] "r" (gGPRS),
          [after] "r" (lGPRS),
          [fid] "r" ((uint64_t)PSCI_AFFINITY_INFO_AARCH64),
          [target_affinity] "r" (affinity)
        : "memory", "cc");

    return ret;
}

void exception_rec_exit_psci_realm(void)
{
    uint64_t index = 0, affinity = 0, mpidr = 0, ret;
    uint64_t lGPRS[EXCEPTION_TEST_MAX_GPRS];
    uint64_t gGPRS[EXCEPTION_TEST_MAX_GPRS];

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    mpidr = 1;

    /* get the second rec affinity*/
    affinity = EXCEPTION_AFFINITY_FROM_MPIDR(mpidr);
    LOG(TEST, "Affinity info: mpidr : 0x%x, affinity : %d  \n", \
              mpidr, affinity);

    ret = exception_capture_psci_affinity_info(affinity, gGPRS, lGPRS);

    /*
     * PSCI_AFFINITY_INFO returns the target CPU state, not PSCI_E_SUCCESS.
     * This probe is used here to force a PSCI REC exit and validate that
     * x7-x30 are restored correctly on resume.
     */
    LOG(TEST, "PSCI_AFFINITY_INFO return value : 0x%x \n", ret);

    for (index = EXCEPTION_TEST_PSCI_GPRS_COMPARE_START;
         index < EXCEPTION_TEST_MAX_GPRS; index++)
    {
        if (gGPRS[index] != lGPRS[index])
        {
            LOG(TEST, "PSCI check, the gprs value is corrupted, \
                              hence testcase failed : line %d\n", __LINE__);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto test_exit;
        }
    }
    /* Check for host rejecting PSCI request */
    ret = val_psci_cpu_on(affinity, val_realm_get_secondary_cpu_entry(), CONTEXT_ID);
    if (ret != PSCI_E_DENIED)
    {
        LOG(ERROR, "Invalid PSCI return code : ret = 0x%x \n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto test_exit;
    }

    LOG(TEST, "REALM PSCI Trigger checks are verified \n");

test_exit:
    val_realm_return_to_host();
}