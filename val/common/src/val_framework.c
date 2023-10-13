/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_framework.h"
#include "val_libc.h"
#include "val_rmm.h"
#include "val_smc.h"

uint64_t security_state;
static uint64_t realm_thread;
uint64_t realm_ipa_width;
uint64_t skip_for_val_logs = 0;

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
 *   @brief    Returns the base address of the shared region
 *   @param    Void
 *   @return   Base address of the shared region
**/
void *val_get_shared_region_base(void)
{
    if (realm_ipa_width != 0)
        return val_get_shared_region_base_ipa(realm_ipa_width);
    else
        return val_get_shared_region_base_pa();
}

/**
 *   @brief    Returns the base address of the shared region
 *   @param    Void
 *   @return   Physical address of the shared region
**/
void *val_get_shared_region_base_pa(void)
{
    return ((void *)(PLATFORM_SHARED_REGION_BASE));
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
 *   @brief    This function prints the given string and data onto the UART
 *   @param    str      - Input String
 *   @param    data1    - Value for first format specifier
 *   @param    data2    - Value for second format specifier
 *   @return   SUCCESS(0)/FAILURE
**/
uint32_t val_printf(const char *msg, uint64_t data1, uint64_t data2)
{
  return pal_printf(msg, data1, data2);
}

/**
 *   @brief    This function checks the security state and take action based on it.
 *   @param    str      - Input String
 *   @param    data1    - Value for first format specifier
 *   @param    data2    - Value for second format specifier
 *   @return   Void
**/
void val_common_printf(const char *msg, uint64_t data1, uint64_t data2)
{
    size_t length = 0, msg_security_state_length = 0;
    char msg_security_state[1000] = {0,};
    uint64_t prev_log_state = (*(uint64_t *)(val_get_shared_region_base() + PRINT_OFFSET));

    if (msg == NULL) {
        LOG(WARN, "\tInvalid Message pointer \n", 0, 0);
        return;
    }

    while (msg[length] != '\0')
    {
        ++length;
    }

    if (length == 0) {
        LOG(WARN, "\tInvalid Message length \n", 0, 0);
        return;
    }

    if (security_state == 1)
    {
        if (prev_log_state != security_state)
        {
            *(uint64_t *)(val_get_shared_region_base() + PRINT_OFFSET) = security_state;
            if (skip_for_val_logs == 1)
            {
                val_memcpy(msg_security_state, "HOST : \n", 8);
            }
        }
    }
    else if (security_state == 2)
    {
        if (prev_log_state != security_state)
        {
            *(uint64_t *)(val_get_shared_region_base() + PRINT_OFFSET) = security_state;
            val_memcpy(msg_security_state, "REALM : \n", 9);
        }
    }
    else if (security_state == 3)
    {
        if (prev_log_state != security_state)
        {
            *(uint64_t *)(val_get_shared_region_base() + PRINT_OFFSET) = security_state;
            val_memcpy(msg_security_state, "SECURE : \n", 10);
        }
    }
    else
    {
        val_memcpy(msg_security_state, "UNKNOWN : \n", 11);
    }

    msg_security_state_length = val_strlen(msg_security_state);

    val_memcpy(&msg_security_state[msg_security_state_length], msg, length);

    if (security_state == 2)
    {
        __attribute__((aligned (PAGE_SIZE))) val_print_rsi_host_call_t realm_print;
            /* Write realm message to shared printf location */
        val_memcpy((char *)(val_get_shared_region_base() + REALM_PRINTF_MSG_OFFSET),
                    (char *)msg_security_state,
                    length+msg_security_state_length+1);
        *(uint64_t *)(val_get_shared_region_base() + REALM_PRINTF_DATA1_OFFSET) = (uint64_t)data1;
        *(uint64_t *)(val_get_shared_region_base() + REALM_PRINTF_DATA2_OFFSET) = (uint64_t)data2;

        /* Print from realm through RSI_HOST_CALL */

        realm_print.imm = VAL_REALM_PRINT_MSG;
        val_smc_call(RSI_HOST_CALL, (uint64_t)&realm_print, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    }
    else {
        val_printf(msg_security_state, data1, data2);
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
 *   @brief    Records the state and status of test
 *   @param    status - Test status bit field - (state|status_code)
 *   @return   void
**/
void val_set_status(uint32_t status)
{
    uint8_t state = ((status >> TEST_STATE_SHIFT) & TEST_STATE_MASK);
    val_test_status_buffer_ts *curr_test_status = (val_get_shared_region_base()
                                                  + TEST_STATUS_OFFSET);

    /* Update the test_status only when previously set status isn't fail or
     * it is required to set as part of test init */
    if ((curr_test_status->state != TEST_FAIL && curr_test_status->state != TEST_ERROR)
            || (state == TEST_START))

    {
        curr_test_status->state = state;
        curr_test_status->status_code  = (status & TEST_STATUS_CODE_MASK);
    }
}

/**
 *   @brief    Returns the state and status for a given test
 *   @param    Void
 *   @return   test status
**/
uint32_t val_get_status(void)
{
    val_test_status_buffer_ts *curr_test_status = (val_get_shared_region_base()
                                                   + TEST_STATUS_OFFSET);
    return (uint32_t)(((curr_test_status->state) << TEST_STATE_SHIFT) |
            (curr_test_status->status_code));
}

/**
 *    @brief     Writes 'size' bytes from buffer into non-volatile memory at a given
 *               'base + offset'.
 *    @param     offset    - Offset
 *    @param     buffer    - Pointer to source address
 *    @param     size      - Number of bytes
 *    @return    SUCCESS/FAILURE
**/
uint32_t val_nvm_write(uint32_t offset, void *buffer, size_t size)
{
      return pal_nvm_write(offset, buffer, size);
}

/**
 *   @brief     Reads 'size' bytes from Non-volatile memory 'base + offset' into given buffer.
 *   @param     offset    - Offset from NV MEM base address
 *   @param     buffer    - Pointer to destination address
 *   @param     size      - Number of bytes
 *   @return    SUCCESS/FAILURE
**/
uint32_t val_nvm_read(uint32_t offset, void *buffer, size_t size)
{
      return pal_nvm_read(offset, buffer, size);
}

/**
 *   @brief    Initializes and enable the hardware watchdog timer
 *   @param    void
 *   @return   SUCCESS/FAILURE
 **/
uint32_t val_watchdog_enable(void)
{
      return pal_watchdog_enable();
}

/**
 *   @brief    Disables the hardware watchdog timer
 *   @param    void
 *   @return   SUCCESS/FAILURE
 **/
uint32_t val_watchdog_disable(void)
{
      return pal_watchdog_disable();
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
