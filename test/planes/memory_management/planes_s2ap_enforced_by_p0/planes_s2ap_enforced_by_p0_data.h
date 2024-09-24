/*
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */


#include "val_realm_planes.h"
#include "val_realm_framework.h"

/* Test IPA memory layout
 *
 * TEST IPA : 0x1000 (2nd entry in L3 table)
 *
 * ipa: 0x1000    HIPAS,RIPAS   P1 permission  P2 Permission
 * --------------------------------------------------------
 * | CODE_PAGE,  ASSIGNED,RAM     NoAccess     NoAccess   |
 * --------------------------------------------------------
 * | CODE_PAGE,  ASSIGNED,RAM      RO+upX       RO+upX    |
 * --------------------------------------------------------
 * | CODE_PAGE,  ASSIGNED,RAM      RO+upX       RO+upX    |
 * --------------------------------------------------------
 * | CODE_PAGE,  ASSIGNED,RAM      RO+upX       RO+upX    |
 * --------------------------------------------------------
 * | DATA_PAGE,  ASSIGNED,RAM        RO        NoAccess   |
 * --------------------------------------------------------
 * | DATA_PAGE,  ASSIGNED,RAM        RW        NoAccess   |
 * --------------------------------------------------------
 * | DATA_PAGE,  ASSIGNED,RAM        WO        NoAccess   |
 * --------------------------------------------------------
 * | DATA_PAGE,  ASSIGNED,RAM     NoAccess        RO      |
 * --------------------------------------------------------
 * | DATA_PAGE,  ASSIGNED,RAM     NoAccess        RO      |
 * --------------------------------------------------------
 * | DATA_PAGE,  ASSIGNED,RAM     NoAccess        RO      |
 * --------------------------------------------------------
 * ipa: 0xB000
 *
 *                         (........)
 *
 */

#define HVC_OPCODE 0xD4000002 /* Opcode for HVC */
#define LDR_OPCODE 0xF9400001 /* Opcode for LDR x1, [x0] */
#define STR_OPCODE 0xF9000001 /* Opcode for STR x1, [x0] */

#define PLANE_COUNT 2
#define TEST_PAGE_COUNT 10
#define CODE_PAGE_COUNT 4
#define NUM_PERM_INDEX 8

#define CODE_1_OFFSET 0x0
#define CODE_2_OFFSET 0x1000
#define CODE_3_OFFSET 0x2000
#define CODE_4_OFFSET 0x3000
#define DATA_1_OFFSET 0x4000
#define DATA_2_OFFSET 0x5000
#define DATA_3_OFFSET 0x6000
#define DATA_4_OFFSET 0x7000
#define DATA_5_OFFSET 0x8000
#define DATA_6_OFFSET 0x9000

#define PERMISSION_INDEX_1 1
#define PERMISSION_INDEX_2 2
#define PERMISSION_INDEX_3 3
#define PERMISSION_INDEX_4 4
#define PERMISSION_INDEX_5 5
#define PERMISSION_INDEX_6 6
#define PERMISSION_INDEX_7 7
#define PERMISSION_INDEX_8 8

typedef struct {
    uint8_t count;
    uint32_t opcodes[2];
} Instructions;

typedef struct {
    uint64_t entry_pc;
    uint64_t entry_x0;
    uint64_t esr_el2_ec[PLANE_COUNT];
} test_stimulus;

uint64_t overlay_val[NUM_PERM_INDEX][PLANE_COUNT] = {
    {S2_AP_NO_ACCESS, S2_AP_NO_ACCESS},
    {S2_AP_RO_upX, S2_AP_RO_upX},
    {S2_AP_RO, S2_AP_NO_ACCESS},
    {S2_AP_RW, S2_AP_NO_ACCESS},
    {S2_AP_WO, S2_AP_NO_ACCESS},
    {S2_AP_NO_ACCESS, S2_AP_RO},
    {S2_AP_NO_ACCESS, S2_AP_RW},
    {S2_AP_NO_ACCESS, S2_AP_WO}
};

uint64_t PermList[TEST_PAGE_COUNT] = {
    PERMISSION_INDEX_1,
    PERMISSION_INDEX_2,
    PERMISSION_INDEX_2,
    PERMISSION_INDEX_2,
    PERMISSION_INDEX_3,
    PERMISSION_INDEX_4,
    PERMISSION_INDEX_5,
    PERMISSION_INDEX_6,
    PERMISSION_INDEX_7,
    PERMISSION_INDEX_8
};

Instructions inst_list[4] = {
    {1, {HVC_OPCODE} },
    {1, {HVC_OPCODE} },
    {2, {LDR_OPCODE, HVC_OPCODE} },
    {2, {STR_OPCODE, HVC_OPCODE} }
};

test_stimulus test_data[] = {
    {CODE_1_OFFSET, 0, {ESR_EL2_EC_INST_ABORT, ESR_EL2_EC_INST_ABORT} },
    {CODE_2_OFFSET, 0, {ESR_EL2_EC_HVC, ESR_EL2_EC_HVC} },
    {CODE_3_OFFSET, DATA_1_OFFSET, {ESR_EL2_EC_HVC, ESR_EL2_EC_DATA_ABORT} },
    {CODE_3_OFFSET, DATA_2_OFFSET, {ESR_EL2_EC_HVC, ESR_EL2_EC_DATA_ABORT} },
    {CODE_3_OFFSET, DATA_3_OFFSET, {ESR_EL2_EC_DATA_ABORT, ESR_EL2_EC_DATA_ABORT} },
    {CODE_3_OFFSET, DATA_4_OFFSET, {ESR_EL2_EC_DATA_ABORT, ESR_EL2_EC_HVC} },
    {CODE_3_OFFSET, DATA_5_OFFSET, {ESR_EL2_EC_DATA_ABORT, ESR_EL2_EC_HVC} },
    {CODE_3_OFFSET, DATA_6_OFFSET, {ESR_EL2_EC_DATA_ABORT, ESR_EL2_EC_DATA_ABORT} },
    {CODE_4_OFFSET, DATA_1_OFFSET, {ESR_EL2_EC_DATA_ABORT, ESR_EL2_EC_DATA_ABORT} },
    {CODE_4_OFFSET, DATA_2_OFFSET, {ESR_EL2_EC_HVC, ESR_EL2_EC_DATA_ABORT} },
    {CODE_4_OFFSET, DATA_3_OFFSET, {ESR_EL2_EC_HVC, ESR_EL2_EC_DATA_ABORT} },
    {CODE_4_OFFSET, DATA_4_OFFSET, {ESR_EL2_EC_DATA_ABORT, ESR_EL2_EC_DATA_ABORT} },
    {CODE_4_OFFSET, DATA_5_OFFSET, {ESR_EL2_EC_DATA_ABORT, ESR_EL2_EC_HVC} },
    {CODE_4_OFFSET, DATA_6_OFFSET, {ESR_EL2_EC_DATA_ABORT, ESR_EL2_EC_HVC} }
};

