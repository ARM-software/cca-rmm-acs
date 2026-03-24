/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef _RHI_SESSION_SEND_H_
#define _RHI_SESSION_SEND_H_

enum action_type {
    ACTION_OPEN = 0,
    ACTION_SEND = 1,
    ACTION_CLOSE = 2
};

struct session_step {
    enum action_type action;
    uint64_t sess_id;
    uint64_t conn_type;
    uint64_t expected_status;
    uint64_t expected_state;
    uint64_t expected_len;
};

#endif /* _RHI_SESSION_SEND_H_ */
