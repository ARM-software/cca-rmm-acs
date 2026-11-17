/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"
#include "pal_arch_helpers.h"
#include "val_realm_framework.h"

void gic_trapped_icc_sysreg_access_realm(void)
{
    write_icv_dir_el1(SPI_vINTID);

    LOG(ERROR, "ICV_DIR_EL1 write did not trap to host\n");
    val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
    val_realm_return_to_host();
}
