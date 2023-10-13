 /*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __EXCEPTION_COMMON_HOST__
#define __EXCEPTION_COMMON_HOST__

#include "test_database.h"
#include "val_host_framework.h"
#include "val_mp_supp.h"
#include "val_host_rmi.h"

#define EXCEPTION_MASK(regfield) \
    ((~0UL >> (64UL - (regfield##_WIDTH))) << (regfield##_SHIFT))

/* recoverable */
#define ESR_EL2_ISS_SET_UER 0
/* Uncontainable*/
#define ESR_EL2_ISS_SET_UC 2
/* Restartable state */
#define ESR_EL2_ISS_SET_UE0 3

#define ESR_EL2_ISS_SET_SHIFT 11
#define ESR_EL2_ISS_SET_MASK (0x3 << 11)
#define ESR_EL2_ISS_SET(iss) ((iss & ESR_EL2_ISS_SET_MASK) >> ESR_EL2_ISS_SET_SHIFT)

#define ESR_EL2_ISS_EA_SHIFT 9
#define ESR_EL2_ISS_EA_MASK (0x3 << 9)
#define ESR_EL2_ISS_EA(iss) ((iss & ESR_EL2_ISS_EA_MASK) >> ESR_EL2_ISS_EA_SHIFT)

#define ESR_EL2_ISS_EXCEPTION_SHIFT    0
#define ESR_EL2_ISS_EXCEPTION_WIDTH    25
#define ESR_EL2_ISS_EXCEPTION_MASK    EXCEPTION_MASK(ESR_EL2_ISS_EXCEPTION)

#define ESR_EL2_ISS_IFSC_MASK (0x3F)
#define ESR_EL2_ISS_IFSC(iss) (iss & ESR_EL2_ISS_IFSC_MASK)

/* HPFAR_EL2 definitions */
#define ESR_EL2_HPFAR_OFFSET    8
#define ESR_EL2_HPFAR_SHIFT        4
#define ESR_EL2_HPFAR_WIDTH        40
#define ESR_EL2_HPFAR_MASK        EXCEPTION_MASK(ESR_EL2_HPFAR)

#define ESR_EL2_ISS_IA_TFSC_L3_TRANS_FAULT 0x07

typedef enum _exception_ripas_exit_intent {
    EXCEPTION_RIPAS_EXIT_ACCEPT,
    EXCEPTION_RIPAS_EXIT_REJECT,
    EXCEPTION_RIPAS_EXIT_PARTIAL
} exception_ripas_exit_intent_te;

typedef struct _exception_validate_ripas_exit {
    uint64_t ripas_base;
    uint64_t ripas_top;
    uint64_t ripas_value;
    exception_ripas_exit_intent_te exception_ripas_exit_intent;
    uint64_t test_fault_addr;
    val_host_realm_ts *realm;
} exception_rec_exit_ts;

void exception_get_ec_imm(uint64_t esr, uint64_t *ec, uint64_t *imm);
void exception_copy_exit_to_entry(val_host_rec_entry_ts *rec_entry,
                                          val_host_rec_exit_ts *rec_exit);
uint64_t exception_validate_rec_exit_ripas(\
            exception_rec_exit_ts ripas_rec_exit);
#endif /* #ifndef __EXCEPTION_COMMON_HOST__ */
