/** @file
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
**/

#include <stdio.h>
#include <stdint.h>
#include "platform_override_fvp.h"

#ifndef _PAL_OVERRIDE_STRUCT_H_
#define _PAL_OVERRIDE_STRUCT_H_

typedef enum {
    MMIO = 0,
    IO = 1
} BAR_MEM_INDICATOR_TYPE;

typedef enum {
    BITS_32 = 0,
    BITS_64 = 2
} BAR_MEM_DECODE_TYPE;

typedef struct {
    uint32_t bdf;
    uint32_t rp_bdf;
} device_attr;

typedef struct {
    uint32_t num_entries;
    device_attr device[];         /* in the format of Segment/Bus/Dev/Func */
} exerciser_device_bdf_table;

typedef enum {
    MMIO_PREFETCHABLE = 0x0,
    MMIO_NON_PREFETCHABLE = 0x1
} BAR_MEM_TYPE;

struct exerciser_data_bar_space {
    void *base_addr;
    BAR_MEM_TYPE type;
};

struct ecam_reg_data {
    uint32_t offset;    /* Offset into 4096 bytes ecam config reg space */
    uint32_t attribute;
    uint32_t value;
};

struct exerciser_data_cfg_space {
    struct ecam_reg_data reg[10];
};

typedef union exerciser_data {
    struct exerciser_data_cfg_space cfg_space;
    struct exerciser_data_bar_space bar_space;
} exerciser_data_t;

typedef enum {
    EXERCISER_DATA_CFG_SPACE = 0x1,
    EXERCISER_DATA_BAR0_SPACE = 0x2,
    EXERCISER_DATA_MMIO_SPACE = 0x3,
} EXERCISER_DATA_TYPE;


typedef enum {
    ACCESS_TYPE_RD = 0x0,
    ACCESS_TYPE_RW = 0x1
} ECAM_REG_ATTRIBUTE;

typedef enum {
    TYPE0 = 0x0,
    TYPE1 = 0x1,
} EXERCISER_CFG_HEADER_TYPE;


typedef enum {
    CFG_READ   = 0x0,
    CFG_WRITE  = 0x1,
} EXERCISER_CFG_TXN_ATTR;


typedef enum {
    TXN_REQ_ID     = 0x0,
    TXN_ADDR_TYPE  = 0x1,
    TXN_REQ_ID_VALID    = 0x2,
} EXERCISER_TXN_ATTR;

typedef enum {
    AT_UNTRANSLATED = 0x0,
    AT_TRANS_REQ    = 0x1,
    AT_TRANSLATED   = 0x2,
    AT_RESERVED     = 0x3
} EXERCISER_TXN_ADDR_TYPE;

typedef enum {
    DEVICE_nGnRnE = 0x0,
    DEVICE_nGnRE  = 0x1,
    DEVICE_nGRE   = 0x2,
    DEVICE_GRE    = 0x3
} ARM_DEVICE_MEM;

typedef enum {
    NORMAL_NC = 0x4,
    NORMAL_WT = 0x5
} ARM_NORMAL_MEM;

uint32_t pal_exerciser_set_param(EXERCISER_PARAM_TYPE Type, uint64_t Value1, uint64_t Value2,
                                                                               uint32_t Bdf);
uint32_t pal_exerciser_set_state (EXERCISER_STATE State, uint64_t *Value, uint32_t Instance);
uint32_t pal_exerciser_get_data(EXERCISER_DATA_TYPE Type, exerciser_data_t *Data, uint32_t Bdf,
                                                                                uint64_t Ecam);
uint32_t pal_exerciser_start_dma_direction (uint64_t Base, EXERCISER_DMA_ATTR Direction);
uint32_t pal_exerciser_get_param(EXERCISER_PARAM_TYPE Type, uint64_t *Value1, uint64_t *Value2,
                                                                                 uint32_t Bdf);
uint32_t pal_exerciser_get_state(EXERCISER_STATE *State, uint32_t Bdf);
uint32_t pal_exerciser_ops(EXERCISER_OPS Ops, uint64_t Param, uint32_t Bdf);

#endif
