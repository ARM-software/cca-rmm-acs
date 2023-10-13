/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_secure_framework.h"
#include "test_database.h"
#include "val_irq.h"
#include "pal_interfaces.h"

extern const uint32_t  total_tests;
extern const test_db_t test_list[];

/**
 *   @brief    Send control back to normal world during test execution phase
 *             and waits for message from normal world
 *   @param    void
 *   @return   SUCCESS(0)/FAILURE
**/
uint32_t val_secure_return_to_host(void)
{
    if (pal_sync_resp_call_to_host())
    {
        VAL_PANIC("\tpal_sync_resp_call_to_host failed\n");
    }
    return VAL_SUCCESS;
}

/**
 *   @brief    Send control back to normal world during test execution phase
 *             and waits for message from normal world
 *   @param    void
 *   @return   SUCCESS(0)/FAILURE
**/
uint32_t val_secure_return_to_preempted_host(void)
{
    if (pal_sync_resp_call_to_preempted_host())
    {
        VAL_PANIC("\tpal_sync_resp_call_to_host failed\n");
    }
    return VAL_SUCCESS;
}

/**
 *   @brief    Finds current test index
 *   @param    void
 *   @return   0
**/
static uint32_t val_secure_find_current_test_index(void)
{
    uint32_t          index;

    for (index = 1; index < total_tests ; index++)
    {
        /* Is this the current test */
        if (!val_is_current_test((void *)test_list[index].test_name))
            return index;
        else
            continue;
    }
    VAL_PANIC("No valid secure test found, something went wrong\n");
    return 0;
}

/**
 *   @brief    Query test database and execute test from each suite one by one
 *   @param    void
 *   @return   void
**/
static void val_secure_test_dispatch(void)
{
    test_fptr_t       fn_ptr;

    /* Register Secure service with EL3/EL2 Software */
    if (pal_register_acs_service())
    {
        VAL_PANIC("\tpal_register_acs_service failed\n");
    }

    /* Wait for message from Normal world */
    if (pal_wait_for_sync_call())
    {
        VAL_PANIC("\tpal_wait_for_sync_call failed\n");
    }

    while (1)
    {
        fn_ptr = (test_fptr_t)(test_list[val_secure_find_current_test_index()].secure_fn);
        if (fn_ptr == NULL)
        {
            VAL_PANIC("Invalid secure test address\n");
        }

        /* Execute secure test */
        fn_ptr();

        /* Send control back to normal world and wait for message */
        val_secure_return_to_host();
    }

}

/**
 *   @brief    C entry function for endpoint
 *   @param    primary_cpu_boot     - Boolean value for primary cpu boot
 *   @return   void (Never returns)
**/
void val_secure_main(bool primary_cpu_boot)
{
    val_set_security_state_flag(3);

    xlat_ctx_t  *secure_xlat_ctx = val_secure_get_xlat_ctx();

    if (primary_cpu_boot == true)
    {
        /* Add secure region into TT data structure */

        val_secure_add_mmap();

        /* Write page tables */
        val_setup_mmu(secure_xlat_ctx);

        val_irq_setup();

        /* Physical IRQ interrupts are taken to EL2, unless they are routed to EL3.*/
        write_hcr_el2((1 << 27)); //TGE=1
    }

    /* Enable Stage-1 MMU */
    val_enable_mmu(secure_xlat_ctx);

    /* Ready to run test regression */
    val_secure_test_dispatch();

    LOG(ALWAYS, "SECURE : Entering standby.. \n", 0, 0);
    pal_terminate_simulation();
}
