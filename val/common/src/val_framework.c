/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_framework.h"
#include "val_libc.h"
#include "val_rmm.h"
#include "val_smc.h"
#include "val_hvc.h"

uint64_t security_state;
static uint64_t realm_thread;
uint64_t realm_ipa_width;
uint64_t skip_for_val_logs = 0;
bool realm_in_p0 = false;
bool realm_in_pn = false;

/**
 *   @brief    set the security state
 *   @param    Void
 *   @return   Void
**/
void val_set_security_state_flag(uint64_t state)
{
    security_state = state;
}

/**
 *   @brief    This is called only from realm image
 *   @param    Void
 *   @return   Void
**/
void val_set_running_in_realm_flag(void)
{
    realm_thread = 1;
}

/**
 *   @brief    Returns the IPA address of the shared region
 *   @param    ipa_width      - Realm IPA_WIDTH
 *   @return   IPA address of the shared region
**/
void *val_get_shared_region_base_ipa(uint64_t ipa_width)
{
    if (realm_thread == 1)
        realm_ipa_width = ipa_width;

    // For realm, ns region must be mapped in unprotected space
    return ((void *)(VAL_NS_SHARED_REGION_IPA_OFFSET | (1ull << (ipa_width - 1))));
}

/**
 *   @brief    Returns the IPA address for given PA
 *   @param    ipa_width      - Realm IPA_WIDTH
 *   @param    pa             - Shared region physical address
 *   @return   IPA address of the shared region
**/
uint64_t val_get_ns_shared_region_base_ipa(uint64_t ipa_width, uint64_t pa)
{
    // For realm, ns region must be mapped in unprotected space
    return ((pa | (1ull << (ipa_width - 1))));
}

/**
 * @brief Prints a formatted message from the Realm environment to a shared memory region.
 *
 * This function formats a message using a variable argument list and writes it to a
 * designated shared memory buffer. It then triggers SMC or HVC to relay the message
 * to the host environment, depending on the execution
 * context.
 *
 * @param verbosity The verbosity level of the log message.
 * @param fmt The format string, similar to printf.
 * @param ... The variable argument list corresponding to the format specifiers.
 **/
void val_realm_printf(print_verbosity_t verbosity, const char *fmt, ...)
{
    va_list args;
    __attribute__((aligned (PAGE_SIZE))) val_print_rsi_host_call_t realm_print;

    char *log_buffer = (char *)(val_get_shared_region_base() + REALM_PRINTF_MSG_OFFSET);

    /* Write realm message to shared printf location */
    *(print_verbosity_t *)(val_get_shared_region_base() + \
                            REALM_PRINTF_VERBOSITY_OFFSET) = (print_verbosity_t)verbosity;

    va_start(args, fmt);

    (void)val_vsnprintf((char *)log_buffer,
            MAX_BUF_SIZE, fmt, args);

    va_end(args);

    /* Print from realm through RSI_HOST_CALL if in P0 or HVC call to P0 if executing in Pn */
    if (realm_in_p0) {
        realm_print.imm = VAL_REALM_PRINT_MSG;
        val_smc_call(RSI_HOST_CALL, (uint64_t)&realm_print, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    } else {
        val_hvc_call(PSI_PRINT_MSG, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
}

/**
 *   @brief    This function prints the security state as a prefix to ACS prints.
 *   @param    none
 *   @return   none
**/
void val_print_secuity_state(void)
{
    uint64_t prev_log_state = (*(uint64_t *)(val_get_shared_region_base() + PRINT_OFFSET));

    if (security_state == SEC_STATE_NS)
    {
        if (prev_log_state != security_state)
        {
            *(uint64_t *)(val_get_shared_region_base() + PRINT_OFFSET) = security_state;
            if (skip_for_val_logs == 1)
            {
                LOG(ALWAYS, "Host:\n");
            }
        }
    }

    else if (security_state == SEC_STATE_REALM)
    {
        if (prev_log_state != security_state)
        {
            *(uint64_t *)(val_get_shared_region_base() + PRINT_OFFSET) = security_state;
            LOG(ALWAYS, "Realm:\n");
        }
    }
    else if (security_state == SEC_STATE_SECURE)
    {
        if (prev_log_state != security_state)
        {
            *(uint64_t *)(val_get_shared_region_base() + PRINT_OFFSET) = security_state;
            LOG(ALWAYS, "Secure:\n");
        }
    }
    else
    {
        LOG(ALWAYS, "Unknown:\n");
    }
}


/**
 *   @brief    Returns the current test number from shared region
 *   @param    Void
 *   @return   Current test number
**/
uint32_t val_get_curr_test_num(void)
{
    return (*(uint32_t *)((val_get_shared_region_base() + TEST_NUM_OFFSET)));
}

/**
 *   @brief    Sets the current test number into shared region
 *   @param    test_num   - Current test number
 *   @return   Void
**/
void val_set_curr_test_num(uint32_t test_num)
{
    *(uint32_t *)(val_get_shared_region_base() + TEST_NUM_OFFSET) = (uint32_t)test_num;
}

/**
 *   @brief    Compare the given string with current
 *             test name stored into the shared memory by host
 *   @param    testname   - Test name
 *   @return   SUCCESS(0)/FAILURE
**/
uint32_t val_is_current_test(char *testname)
{
    size_t length = 0;
    char   *shared_test_name = (void *)(val_get_shared_region_base() + TEST_NAME_OFFSET);

    while (shared_test_name[length] != '\0')
    {
      ++length;
    }

    if (val_memcmp((void *)testname, (void *)shared_test_name, length))
        return VAL_ERROR;

    return VAL_SUCCESS;
}

/**
 *   @brief    Sets the current test name into shared region
 *   @param    testname   - Current test name string
 *   @return   Void
**/
void val_set_curr_test_name(char *testname)
{
   size_t length = 0;

    while (testname[length] != '\0')
    {
      ++length;
    }

    /* Write test name to shared printf location */
    val_memcpy((char *)(val_get_shared_region_base() + TEST_NAME_OFFSET),
                (char *)testname,
                length+1);

}

/**
 *   @brief    Enables the NS watchdog timer
 *   @param    ms     : time out value
 *   @return   void
 **/
void val_ns_wdog_enable(uint32_t ms)
{
    pal_ns_wdog_enable(ms);
}

/**
 *   @brief    Disables the NS watchdog timer
 *   @param    void
 *   @return   void
 **/
void val_ns_wdog_disable(void)
{
    pal_ns_wdog_disable();
}

/**
 *   @brief    Enables the trusted watchdog timer
 *   @param    ms     - time out value
 *   @return   SUCCESS/FAILURE
 **/
uint32_t val_twdog_enable(uint32_t ms)
{
    return pal_twdog_enable(ms);
}

/**
 *   @brief    Disables the trusted watchdog timer
 *   @param    void
 *   @return   SUCCESS/FAILURE
 **/
uint32_t val_twdog_disable(void)
{
    return pal_twdog_disable();
}

/**
 *   @brief    Returns the base address of the NS UART
 *   @param    void
 *   @return   Base address of the NS UART
 **/
void *val_get_ns_uart_base(void)
{
      return ((void *)PLATFORM_NS_UART_BASE);
}

/**
 *   @brief    Returns the base address of the secure image
 *   @param    void
 *   @return   Base address of the secure image
 **/
void *val_get_secure_base(void)
{
      return ((void *)PLATFORM_SECURE_IMAGE_BASE);
}
