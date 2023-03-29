/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_EXCEPTIONS_H_
#define _VAL_EXCEPTIONS_H_

#include "val_framework.h"
#include "val_sysreg.h"

#define EC_DATA_ABORT_SAME_EL        0x25  /* EC = 100101, Data abort. */
#define EC_INSTRUCTION_ABORT_SAME_EL 0x21  /* EC = 100001, Instruction abort. */

bool val_irq_current(void);
bool val_sync_exception_current(void);
void val_exception_setup(void (*irq)(void), bool (*exception)(void));
#endif /* _VAL_EXCEPTIONS_H_ */
