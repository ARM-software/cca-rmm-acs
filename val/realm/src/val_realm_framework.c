/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_realm_framework.h"
#include "val_smc.h"
#include "test_database.h"
#include "val_irq.h"
#include "val_realm_rsi.h"
#include "val.h"
#include "val_realm_memory.h"

extern uint64_t realm_ipa_width;
extern uint64_t val_image_load_offset;
extern const test_db_t test_list[];

/**
 *   @brief    Return secondary cpu entry address
 *   @param    void
 *   @return   Secondary cpu entry address
**/
uint64_t val_realm_get_secondary_cpu_entry(void)
{
    return (uint64_t)&acs_realm_entry;
}

/**
 *   @brief    This function prints the given string and data onto the uart
 *   @param    msg      - Input String
 *   @param    data1    - Value for first format specifier
 *   @param    data2    - Value for second format specifier
 *   @return   SUCCESS(0)/FAILURE
**/
uint32_t val_realm_printf(const char *msg, uint64_t data1, uint64_t data2)
{
    size_t length = 0;

    while (msg[length] != '\0')
    {
      ++length;
    }

    /* Write realm message to shared printf location */
    val_memcpy((char *)(val_get_shared_region_base() + REALM_PRINTF_MSG_OFFSET),
                (char *)msg,
                length+1);
    *(uint64_t *)(val_get_shared_region_base() + REALM_PRINTF_DATA1_OFFSET) = (uint64_t)data1;
    *(uint64_t *)(val_get_shared_region_base() + REALM_PRINTF_DATA2_OFFSET) = (uint64_t)data2;

    /* Print from realm through RSI_HOST_CALL */
    val_realm_rsi_host_call((uint16_t)VAL_REALM_PRINT_MSG);
    return VAL_SUCCESS;
}

/**
 *   @brief    Sends control back to host by executing hvc #n
 *   @param    void
 *   @return   void
**/
void val_realm_return_to_host(void)
{
    val_realm_rsi_host_call(VAL_SWITCH_TO_HOST);
    //val_return_to_host_hvc_asm();
}

/**
 *   @brief    Query test database and execute test from each suite one by one
 *   @param    void
 *   @return   void
**/
static void val_realm_test_dispatch(void)
{
    test_fptr_t       fn_ptr;

    fn_ptr = (test_fptr_t)(test_list[val_get_curr_test_num()].realm_fn);
    if (fn_ptr == NULL)
    {
        LOG(ERROR, "Invalid realm test address\n", 0, 0);
        pal_terminate_simulation();
    }

    /* Fix symbol relocation - Add image offset */
    fn_ptr = (test_fptr_t)(fn_ptr + val_image_load_offset);
    /* Execute realm test */
    fn_ptr();

    /* Control shouldn't come here. Test must send control back to host */
}

/**
 *   @brief    C entry function for endpoint
 *   @param    primary_cpu_boot     - Boolean value for primary cpu boot
 *   @return   void (Never returns)
**/
void val_realm_main(bool primary_cpu_boot)
{
    val_set_running_in_realm_flag();
    val_set_security_state_flag(2);

    uint64_t ipa_width = val_realm_get_ipa_width();

    xlat_ctx_t  *realm_xlat_ctx = val_realm_get_xlat_ctx();

    if (primary_cpu_boot == true)
    {
        /* Add realm region into TT data structure */
        val_realm_add_mmap();

        val_realm_update_xlat_ctx_ias_oas(((1UL << ipa_width) - 1), ((1UL << ipa_width) - 1));
        /* Write page tables */
        val_setup_mmu(realm_xlat_ctx);

    }

    /* Enable Stage-1 MMU */
    val_enable_mmu(realm_xlat_ctx);

    val_irq_setup();
    /* Ready to run test regression */
    val_realm_test_dispatch();

    LOG(ALWAYS, "REALM : Entering standby.. \n", 0, 0);
    pal_terminate_simulation();
}
