/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef PAL_SMMUV3_TEST_ENGINE_H
#define PAL_SMMUV3_TEST_ENGINE_H

#include <stdbool.h>
#include "stdint.h"

/*
 * From SMMUv3TestEngine.h 11.30_27
 * Refer to:
 * https://developer.arm.com/documentation/107925/1130/Fast-Models-trace-components/SMMUv3TestEngine
 */

#define NO_SUBSTREAMID    (~0U)
#define SZ_4K             0x00001000
#define SZ_64K            0x00010000

/* From SMMUv3TestEngine.h 11.30_27 */
typedef enum {
    ENGINE_FRAME_MISCONFIGURED = (~0U - 1U),
    ENGINE_ERROR = ~0U,
    ENGINE_NO_FRAME = 0U,
    ENGINE_HALTED = 1U,
    ENGINE_MEMCPY = 2U,
    ENGINE_RAND48 = 3U,
    ENGINE_SUM64 = 4U
} cmd_t;

/* 128 bytes */
typedef struct {
    uint32_t cmd;
    uint32_t uctrl;
    uint32_t count_of_transactions_launched;
    uint32_t count_of_transactions_returned;
    uint64_t msiaddress;
    uint32_t msidata;
    uint32_t msiattr;
    uint32_t attributes;
    uint32_t seed;
    uint64_t begin;
    uint64_t end_incl;
    uint64_t stride;
    uint64_t udata[7];
    uint32_t uctrl1;
    uint32_t uctrl2;

} user_frame_t;

/* 128 bytes */
typedef struct {
    uint32_t pctrl;
    uint32_t downstream_port_index;
    uint32_t streamid;
    uint32_t substreamid;
    uint64_t pctrl1;
    uint64_t pdata[13];
} privileged_frame_t;

/* 128 KiB */
typedef struct {
    user_frame_t user[SZ_64K / sizeof(user_frame_t)];
    privileged_frame_t privileged[SZ_64K / sizeof(privileged_frame_t)];
} engine_pair_t;

int pal_smmu_configure_testengine(uintptr_t bar, uintptr_t source_pa, uintptr_t dest_pa);

#endif /* PAL_SMMUV3_TEST_ENGINE_H */
