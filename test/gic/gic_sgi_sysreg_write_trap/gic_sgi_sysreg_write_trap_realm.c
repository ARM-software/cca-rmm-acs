/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "gic_common.h"
#include "val_realm_framework.h"

void gic_sgi_sysreg_write_trap_realm(void)
{
    write_icc_asgi1r_el1(0U);
    write_icc_sgi0r_el1(0U);
    write_icc_sgi1r(0U);

    LOG(ERROR, "SGI generation register writes did not trap to host\n");
    val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
    val_realm_return_to_host();
}
