/*
 * Copyright (c) 2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"

#define ID_AA64MMFR3_EL1_MEC_SHIFT      U(28)
#define ID_AA64MMFR3_EL1_MEC_MASK       ULL(0xf)

void mec_feat_support_host(void)
{
    uint64_t featreg1;
    uint32_t is_feat_mec_supported = (uint32_t)(val_id_aa64mmfr3_el1_read() >>
        ID_AA64MMFR3_EL1_MEC_SHIFT) & ID_AA64MMFR3_EL1_MEC_MASK;

    if (!is_feat_mec_supported)
    {
        /* On a platform which does not implement FEAT_MEC,MAX_MECID is zero */
        val_host_rmi_features(1, &featreg1);
        if (featreg1 != 0)
        {
            LOG(ERROR, "MECID must be zero if FEAT_MEC not supported\n", 0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto exit;
        }
    } else {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
exit:
    return;
}
