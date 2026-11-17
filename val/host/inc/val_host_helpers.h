/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __VAL_HOST_HELPERS__
#define __VAL_HOST_HELPERS__

#include "test_database.h"
#include "val_host_framework.h"
#include "val_host_rmi.h"

#define GRANULE_SHIFT   12
#define S2TTE_STRIDE    (GRANULE_SHIFT - 3)

/* Data abort ESR fields */
#define ESR_ISS_SET_UER 0x0      /* SET[12:11] = b'00 Recoverable state (UER) */
#define ESR_ISS_EA 0x0           /* EA[9] = b'0 */
#define ESR_ISS_DFSC_TTF_L3 0x7  /* DFSC[5:0] = b'00111 Translation fault, level 3 */
#define ESR_ISS_DFSC_TTF_L2 0x6  /* DFSC[5:0] = b'000110 Translation fault, level 2 */
#define ESR_ISS_DFSC_PF_L3 0xF  /* DFSC[5:0] = b'001111 Permission fault, level 3 */
#define ESR_EC_LOWER_EL 0x24     /* EC[31:26] = b'100100 DA from lower EL */
#define ESR_IA_EC_LOWER_EL 0x20     /* EC[31:26] = b'100000 IA from lower EL */
#define ESR_ISV_VALID 0x1        /* ISV[24] = b'1 ISS[23:14] hold a valid instruction syndrome */
#define ESR_SAS_WORD 0x2         /* SAS[23:22] = b'10 Word */
#define ESR_FnV      0x0        /* FnV[10] = b'0 Valid only if the DFSC code is 0b010000 */
#define ESR_WnR_WRITE 0x1        /* WnR[6] = b'1 Abort caused by writing to a memory location */
#define ESR_WnR_READ 0x0        /* WnR[6] = b'0 Abort caused by an reading from a memory location */
#define ESR_IL_32BIT 0x1        /* IL[25] = b'1 32-bit instruction trapped */

/* Instruction abort ESR fields */
#define ESR_ISS_IFSC_TTF_L3 0x7  /* IFSC[5:0] = b'00111 Translation fault, level 3 */

typedef enum _exception_da_abort_type {
    EMULATABLE_DA,
    NON_EMULATABLE_DA,
    NON_EMULATABLE_DA_UNPROTECTED
} exception_da_abort_type_te;

uint32_t validate_rec_exit_da(val_host_rec_exit_ts *rec_exit, uint64_t hpfar,
                            uint32_t dfsc, exception_da_abort_type_te abort_type, uint32_t wnr);
uint32_t validate_rec_exit_ia(val_host_rec_exit_ts *rec_exit, uint64_t hpfar);
uint64_t val_host_addr_align_to_level(uint64_t addr, uint64_t level);
uint64_t val_host_set_s2ap(val_host_realm_ts *realm);

#endif /* #ifndef __VAL_HOST_HELPERS__ */
