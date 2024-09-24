/*
 *
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#define PLANE_0     0
#define PLANE_1     1

typedef struct{
    bool p0_enable;
    bool p1_enable;
    uint64_t p0_deadline;
    uint64_t p1_deadline;
    uint64_t exp_plane_idx;
} test_stimulus;

static test_stimulus test_data[] = {
    {false, false, 5, 10, PLANE_0},
    {false, false, 10, 5, PLANE_0},
    {false, true,  5, 10, PLANE_1},
    {false, true,  10, 5, PLANE_1},
    {true,  false, 5, 10, PLANE_0},
    {true,  false, 10, 5, PLANE_0},
    {true,  true,  5, 10, PLANE_0},
    {true,  true,  10, 5, PLANE_1}
};
