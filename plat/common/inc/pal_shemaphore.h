/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _PAL_SHEMAPHORE_H_
#define _PAL_SHEMAPHORE_H_

typedef struct s_lock {
    volatile unsigned int lock;
} s_lock_t;

typedef struct {
    /*
     * Counter that keeps track of the minimum number of recipients of the
     * event. When the event is sent, this counter is incremented. When it
     * is received, it is decremented. Therefore, a zero value means that
     * the event hasn't been sent yet, or that all recipients have already
     * received it.
     *
     * Volatile is needed as it will enforce ordering relatively to
     * accesses to the lock.
     */
    volatile unsigned int cnt;

    /* Lock used to avoid concurrent accesses to the counter */
    s_lock_t lock;
} event_t;

void pal_init_spinlock(s_lock_t *lock);
void pal_spin_lock(s_lock_t *lock);
void pal_spin_unlock(s_lock_t *lock);

#endif /* _PAL_SHEMAPHORE_H_ */
