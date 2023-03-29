/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _RTT_LEVEL_START_H_
#define _RTT_LEVEL_START_H_

static uint64_t rtt_sl_start[23][2] = {{32, 0x3ffff000},
                                      {32, 0x7ffff000},
                                      {32, 0xbffff000},
                                      {32, 0xfffff000},
                                      {34, 0x3ffff000},
                                      {34, 0x7ffff000},
                                      {34, 0x23ffff000},
                                      {34, 0x27ffff000},
                                      {34, 0x3bffff000},
                                      {34, 0x3fffff000},
                                      {40, 0x7ffffff000},
                                      {40, 0xfffffff000},
                                      {42, 0x7ffffff000},
                                      {42, 0xfffffff000},
                                      {42, 0x2fffffff000},
                                      {42, 0x3fffffff000},
                                      {52, 0xffffffffffff},
                                      {52, 0x1ffffffffffff},
                                      {52, 0x8ffffffffffff},
                                      {52, 0x9ffffffffffff},
                                      {52, 0xeffffffffffff},
                                      {52, 0xfffffffffffff} };
#endif /* _RTT_LEVEL_START_H_ */

