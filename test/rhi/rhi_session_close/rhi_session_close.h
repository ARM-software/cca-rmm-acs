/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef _RHI_SESSION_CLOSE_H_
#define _RHI_SESSION_CLOSE_H_

enum action_type {
    ACTION_OPEN = 0,
    ACTION_CLOSE = 1
};

struct session_step {
    enum action_type action;
    uint64_t sess_id;
    uint64_t expected;
};

#endif /* _RHI_SESSION_CLOSE_H_ */
