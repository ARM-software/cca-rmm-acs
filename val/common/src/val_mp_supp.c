/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_mp_supp.h"

/* Global variable to store mpidr of primary cpu */
uint64_t val_primary_mpidr = PAL_INVALID_MPID;

/**
 *   @brief    Returns mpidr of primary cpu set during boot.
 *   @param    void
 *   @return   primary mpidr
**/
uint64_t val_get_primary_mpidr(void)
{
    return val_primary_mpidr;
}

/**
 *   @brief    Returns number of cpus in the system.
 *   @param    void
 *   @return   Number of cpus
**/
uint32_t val_get_cpu_count(void)
{
    return pal_get_cpu_count();
}

/**
 *   @brief    Convert mpidr to logical cpu number
 *   @param    mpidr    - mpidr value
 *   @return   Logical cpu number
**/
uint32_t val_get_cpuid(uint64_t mpidr)
{
    uint32_t cpu_index = 0;
    uint32_t total_cpu_num = pal_get_cpu_count();
    uint64_t *phy_mpidr_list = pal_get_phy_mpidr_list_base();

    mpidr = mpidr & PAL_MPIDR_AFFINITY_MASK;

    for (cpu_index = 0; cpu_index < total_cpu_num; cpu_index++)
    {
        if (mpidr == phy_mpidr_list[cpu_index])
            return cpu_index;
    }

    /* In case virtual mpidr returned for realm */
    for (cpu_index = 0; cpu_index < total_cpu_num; cpu_index++)
    {
        if (mpidr == cpu_index)
            return cpu_index;
    }

    return PAL_INVALID_MPID;
}

/**
 *   @brief    Return physical mpidr value of given logical cpu index
 *   @param    cpu_id   - Logical cpu index
 *   @return   physical mpidr value
**/
uint64_t val_get_mpidr(uint32_t cpu_id)
{
    uint64_t *phy_mpidr_list = pal_get_phy_mpidr_list_base();

    if (cpu_id < pal_get_cpu_count())
        return  phy_mpidr_list[cpu_id];
    else
        return PAL_INVALID_MPID;
}

/**
 *   @brief    Return vmpidr value of given logical cpu index
 *   @param    cpu_id   - Logical cpu index
 *   @return   vmpidr value
**/
uint64_t val_get_vmpidr(uint32_t cpu_id)
{
    if (cpu_id < pal_get_cpu_count())
        return  cpu_id;
    else
        return PAL_INVALID_MPID;
}

/*
 * Initialise an event.
 *   event: Address of the event to initialise
 *
 * This function can be used either to initialise a newly created event
 * structure or to recycle one.
 *
 * Note: This function is not MP-safe. It can't use the event lock as it is
 * responsible for initialising it. Care must be taken to ensure this function
 * is called in the right circumstances.
 */
void val_init_event(event_t *event)
{
    event->cnt = 0;
    event->lock.lock = 0;
}

/* Initialize spinlock */
void val_init_spinlock(s_lock_t *lock)
{
    pal_init_spinlock(lock);
}

/* Lock the event */
void val_spin_lock(s_lock_t *lock)
{
    pal_spin_lock(lock);
}

/* Unock the event */
void val_spin_unlock(s_lock_t *lock)
{
    pal_spin_unlock(lock);
}

static void send_event_common(event_t *event, unsigned int inc)
{
    val_spin_lock(&event->lock);
    event->cnt += inc;
    val_spin_unlock(&event->lock);

    //val_dataCacheCleanInvalidateVA((uint64_t)&event->cnt);
    /*
     * Make sure the cnt increment is observable by all CPUs
     * before the event is sent.
     */
    dsbsy();
    sev();
}

/*
 * Send an event to a CPU.
 *   event: Address of the variable that acts as a synchronisation object.
 *
 * Which CPU receives the event is determined on a first-come, first-served
 * basis. If several CPUs are waiting for the same event then the first CPU
 * which takes the event will reflect that in the event structure.
 *
 * Note: This is equivalent to calling:
 *   val_send_event_to(event, 1);
 */
void val_send_event(event_t *event)
{
    LOG(DBG, "Sending event %x\n", (uint64_t) event, 0);
    send_event_common(event, 1);
}

/*
 * Send an event to all CPUs.
 *   event: Address of the variable that acts as a synchronisation object.
 *
 * Note: This is equivalent to calling:
 *   val_send_event_to(event, PLATFORM_NO_OF_CPUS);
 */
void val_send_event_to_all(event_t *event)
{
    //LOG("Sending event %p to all CPUs\n", (void *) event);
    send_event_common(event, val_get_cpu_count());
}

/*
 * Send an event to a given number of CPUs.
 *   event: Address of the variable that acts as a synchronisation object.
 *   cpus_count: Number of CPUs to send the event to.
 *
 * Which CPUs receive the event is determined on a first-come, first-served
 * basis. If more than 'cpus_count' CPUs are waiting for the same event then the
 * first 'cpus_count' CPUs which take the event will reflect that in the event
 * structure.
 */
void val_send_event_to(event_t *event, unsigned int cpus_count)
{
    //LOG("Sending event %p to %u CPUs\n", (void *) event, cpus_count);
    send_event_common(event, cpus_count);
}

/*
 * Wait for an event.
 *   event: Address of the variable that acts as a synchronisation object.
 */
void val_wait_for_event(event_t *event)
{
    unsigned int event_received = 0;

    LOG(DBG, "Waiting for event %x\n", (uint64_t) event, 0);
    while (!event_received) {

        //val_dataCacheInvalidateVA((uint64_t)&event->cnt);
        dsbsy();
        /* Wait for someone to send an event */
        if (!event->cnt) {
            wfe();
        } else {
            val_spin_lock(&event->lock);

             /*
              * Check that the event is still pending and that no
              * one stole it from us while we were trying to
              * acquire the lock.
              */
            if (event->cnt != 0) {
                event_received = 1;
                --event->cnt;
            }
            /*
             * No memory barrier is needed here because val_spin_unlock()
             * issues a Store-Release instruction, which guarantees
             * that loads and stores appearing in program order
             * before the Store-Release are observed before the
             * Store-Release itself.
             */
            val_spin_unlock(&event->lock);
        }
    }

    LOG(DBG, "Event recieved for %x\n", (uint64_t) event, 0);
}
