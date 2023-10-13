/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */


#include "val_exceptions.h"

static void (*irq_callback)(void);
static bool (*exception_callback)(void);

/**
 * Handles an IRQ at the current exception level.
 *
 * Returns false so that the value of elr_el1 is restored from the stack, in
 * case there are nested exceptions.
 */
bool val_irq_current(void)
{
    if (pal_irq_handler_dispatcher())
    {
        LOG(ERROR, "Got unexpected interrupt.\n", 0, 0);
        return false;
    }

    return true;
}

static bool default_sync_current_exception(void)
{
    uint64_t esr = 0, elr = 0, far = 0, ec;
    uint64_t current_el = val_read_current_el();

    if (current_el == EL2)
    {
        esr = val_esr_el2_read();
        elr = val_elr_el2_read();
        far = val_far_el2_read();
    } else if (current_el == EL1)
    {
        esr = val_esr_el1_read();
        elr = val_elr_el1_read();
        far = val_far_el1_read();
    }

    ec = esr >> 26;

    switch (ec)
    {
        case EC_DATA_ABORT_SAME_EL:
            LOG(ERROR, "Data abort: pc=%x, esr=%x", elr, esr);
            LOG(ERROR, ", ec=%x", ec, 0);

            if (!(esr & (1U << 10)))
            { /* Check FnV bit. */
                LOG(ERROR, ", far=%x\n", far, 0);
            } else
            {
                LOG(ERROR, ", far=invalid\n", 0, 0);
            }

            break;

        case EC_INSTRUCTION_ABORT_SAME_EL:
            LOG(ERROR, "Instruction abort: pc=%x, esr=%x", elr, esr);
            LOG(ERROR, ", ec=%x", ec, 0);

            if (!(esr & (1U << 10)))
            { /* Check FnV bit. */
                LOG(ERROR, ", far=%x\n", far, 0);
            } else
            {
                LOG(ERROR, ", far=invalid\n", 0, 0);
            }

            break;

        default:
            LOG(ERROR, "Unknown sync exception elr=%x, esr=%x",
                 elr, esr);
            LOG(ERROR, ", ec=%x\n", ec, 0);

            if (!(esr & (1U << 10)))
            { /* Check FnV bit. */
                LOG(ERROR, ", far=%x\n", far, 0);
            } else
            {
                LOG(ERROR, ", far=invalid\n", 0, 0);
            }
    }

    for (;;)
    {
        /* do nothing */
    }
    return false;
}

/**
 * Handles a synchronous exception at the current exception level.
 *
 * Returns true if the value of elr_el1 should be kept as-is rather than
 * restored from the stack. This enables exception handlers to indicate whether
 * they have changed the value of elr_el1 (e.g., to skip the faulting
 * instruction).
 */
bool val_sync_exception_current(void)
{
    if (exception_callback != NULL)
    {
        return exception_callback();
    }
    return default_sync_current_exception();
}

void val_exception_setup(void (*irq)(void), bool (*exception)(void))
{
    irq_callback = irq;
    exception_callback = exception;
}
