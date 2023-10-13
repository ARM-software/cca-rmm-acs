/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_host_framework.h"
#include "test_database.h"
#include "val_host_realm.h"
#include "val_irq.h"
#include "pal_interfaces.h"
#include "val.h"
#include "val_host_memory.h"

extern const uint32_t  total_tests;
extern const test_db_t test_list[];
extern uint64_t skip_for_val_logs;
/**
 *   @brief    Read realm message from shared printf location and print them using uart
 *   @param    void
 *   @return   SUCCESS(0)/FAILURE
**/
uint32_t val_host_realm_printf_msg_service(void)
{
    /* Read realm message from shared printf location and print them using uart */
    return val_printf((char *)(val_get_shared_region_base() + REALM_PRINTF_MSG_OFFSET),
                *(uint64_t *)(val_get_shared_region_base() + REALM_PRINTF_DATA1_OFFSET),
                *(uint64_t *)(val_get_shared_region_base() + REALM_PRINTF_DATA2_OFFSET));
}

/**
 *   @brief    Executes secure test payload
 *   @param    void
 *   @return   SUCCESS(0)/FAILURE
**/
uint32_t val_host_execute_secure_payload(void)
{
    return pal_sync_req_call_to_secure();
}

/**
 *   @brief    Return secondary cpu entry address
 *   @param    void
 *   @return   Secondary cpu entry address
**/
uint64_t val_host_get_secondary_cpu_entry(void)
{
    return (uint64_t)&acs_host_entry;
}

/**
 *   @brief    Parses input status for a given test and
 *               outputs appropriate information on the console
 *   @param    test_num     -  Test number
 *   @return   Test state
**/
static uint32_t val_host_report_status(uint32_t test_num)
{
    uint32_t status, status_code, state;
    char      test_result_print[PRINT_LIMIT] = "Result";

    (void)test_num;
    status = val_get_status();
    state = (status >> TEST_STATE_SHIFT) & TEST_STATE_MASK;
    status_code = status & TEST_STATUS_CODE_MASK;

    switch (state)
    {
        case TEST_PASS:
            state = TEST_PASS;
            val_strcat(test_result_print, " => Passed\n",
                sizeof(test_result_print));
            break;

        case TEST_SKIP:
            state = TEST_SKIP;
            val_strcat(test_result_print, " => Skipped (Skip code=%d)\n",
                sizeof(test_result_print));
            break;

        case TEST_ERROR:
            state = TEST_ERROR;
            val_strcat(test_result_print, " => Error (Error code=%d)\n",
                sizeof(test_result_print));
            break;
        default:
            state = TEST_FAIL;
            val_strcat(test_result_print, " => Failed (Error Code=%d)\n",
                sizeof(test_result_print));
            break;
    }

    LOG(ALWAYS, test_result_print, status_code, 0);
    LOG(ALWAYS, "\n", 0, 0);
    LOG(ALWAYS, "***********************************\n", 0, 0);
    return state;
}

/**
 *   @brief    This function notifies the framework about test
 *             intension of rebooting the platform. Test returns
 *             to framework on reset.
 *   @param    void
 *   @return   void
**/
void val_host_set_reboot_flag(void)
{
   uint32_t test_progress  = TEST_REBOOTING;

   LOG(INFO, "\tSetting reboot flag\n", 0, 0);
   if (val_nvm_write(VAL_NVM_OFFSET(NVM_TEST_PROGRESS_INDEX),
                 &test_progress, sizeof(uint32_t)))
   {
      VAL_PANIC("\tnvm write failed\n");
   }
}

/**
 *   @brief    This function returns the last run test information
 *   @param    test_info    -   Test information structure pointer
 *   @return   SUCCESS/FAILURE
**/
uint32_t val_host_get_last_run_test_info(val_test_info_ts *test_info)
{
    uint32_t        reboot_run = 0, i = 0;
    uint8_t         test_progress_pattern[] = {TEST_START, TEST_END, TEST_FAIL, TEST_REBOOTING};
    val_regre_report_ts  regre_report = {0};

    if (val_nvm_read(VAL_NVM_OFFSET(NVM_CUR_TEST_NUM_INDEX),
            &test_info->test_num, sizeof(uint32_t)))
        return VAL_ERROR;

    if (val_nvm_read(VAL_NVM_OFFSET(NVM_END_TEST_NUM_INDEX),
            &test_info->end_test_num, sizeof(uint32_t)))
        return VAL_ERROR;

    if (val_nvm_read(VAL_NVM_OFFSET(NVM_TEST_PROGRESS_INDEX),
            &test_info->test_progress, sizeof(uint32_t)))
        return VAL_ERROR;

    LOG(INFO, "\tIn val_host_get_last_run_test_info, test_num=%x\n", test_info->test_num, 0);
    LOG(INFO, "\ttest_progress=%x\n", test_info->test_progress, 0);

    /* Is power on reset or warm reset? Determine based on NVM content */
    while (i < (uint32_t)(sizeof(test_progress_pattern)/sizeof(test_progress_pattern[0])))
    {
        if (test_info->test_progress == test_progress_pattern[i])
        {
            reboot_run = 1;
            break;
        }
        i++;
    }

    /* Power on reset : Initiliase necessary data structure
     * Warm reset : Return previously executed test number
     * */
    if (!reboot_run)
    {
         test_info->test_num         = VAL_INVALID_TEST_NUM;
         test_info->end_test_num     = total_tests;
         test_info->test_progress     = 0;

         if (val_nvm_write(VAL_NVM_OFFSET(NVM_CUR_TEST_NUM_INDEX),
                 &test_info->test_num, sizeof(uint32_t)))
             return VAL_ERROR;
         if (val_nvm_write(VAL_NVM_OFFSET(NVM_END_TEST_NUM_INDEX),
                 &test_info->end_test_num, sizeof(uint32_t)))
             return VAL_ERROR;
         if (val_nvm_write(VAL_NVM_OFFSET(NVM_TEST_PROGRESS_INDEX),
                 &test_info->test_progress, sizeof(uint32_t)))
             return VAL_ERROR;
         if (val_nvm_write(VAL_NVM_OFFSET(NVM_TOTAL_PASS_INDEX),
                 &regre_report.total_pass, sizeof(uint32_t)))
             return VAL_ERROR;
         if (val_nvm_write(VAL_NVM_OFFSET(NVM_TOTAL_FAIL_INDEX),
                 &regre_report.total_fail, sizeof(uint32_t)))
             return VAL_ERROR;
         if (val_nvm_write(VAL_NVM_OFFSET(NVM_TOTAL_SKIP_INDEX),
                 &regre_report.total_skip, sizeof(uint32_t)))
             return VAL_ERROR;
         if (val_nvm_write(VAL_NVM_OFFSET(NVM_TOTAL_ERROR_INDEX),
                 &regre_report.total_error, sizeof(uint32_t)))
             return VAL_ERROR;
    }

    LOG(INFO, "\tIn val_host_get_last_run_test_num, test_num=%x\n", test_info->test_num, 0);
    LOG(INFO, "\tregre_report.total_pass=%x\n", regre_report.total_pass, 0);
    LOG(INFO, "\tregre_report.total_fail=%x\n", regre_report.total_fail, 0);
    LOG(INFO, "\tregre_report.total_skip=%x\n", regre_report.total_skip, 0);
    LOG(INFO, "\tregre_report.total_error=%x\n", regre_report.total_error, 0);
    return VAL_SUCCESS;
}

/**
 * @brief  This API prints the testname and sets the test
 *           state to invalid.
 * @param  test_num     -   Test number
 * @return void
**/

static void val_host_test_init(uint32_t test_num)
{
   char testname[PRINT_LIMIT] = "";
   uint32_t test_progress = TEST_START;

   /* Clear test status */
   val_set_status(RESULT_START(VAL_STATUS_INVALID));

   /* Save current test num and testname */
   val_set_curr_test_num(test_num);
   val_set_curr_test_name((char *)test_list[test_num].test_name);
   LOG(DBG, "test_num=%d\n", val_get_curr_test_num(), 0);

   val_strcat(testname,
                (char *)test_list[test_num].suite_name,
                sizeof(testname));
   val_strcat(testname,
                (char *)test_list[test_num].test_name,
                sizeof(testname));
   val_strcat(testname, "\n", sizeof(testname));

   LOG(ALWAYS, "\n", 0, 0);
   LOG(ALWAYS, testname, 0, 0);

   if (val_nvm_write(VAL_NVM_OFFSET(NVM_TEST_PROGRESS_INDEX),
           &test_progress, sizeof(uint32_t)))
   {
      VAL_PANIC("\tnvm write failed\n");
   }

   if (val_watchdog_enable())
   {
      VAL_PANIC("\tWatchdog enable failed\n");
   }

   /* Reset mem alloc data structure */
   val_host_mem_alloc_init();
}

/**
 * @brief  This API prints the final test result
 * @param  void
 * @return void
**/
static void val_host_test_exit(void)
{
   uint32_t test_progress = TEST_END;

#if defined(TEST_COMBINE)
   if (val_host_postamble())
   {
         LOG(ERROR, "\tval_host_postamble failed\n", 0, 0);
         val_set_status(RESULT_FAIL(VAL_ERROR));
   }
#endif

   if (val_watchdog_disable())
   {
      VAL_PANIC("\tWatchdog disable failed\n");
   }

   if (val_nvm_write(VAL_NVM_OFFSET(NVM_TEST_PROGRESS_INDEX),
           &test_progress, sizeof(uint32_t)))
   {
      VAL_PANIC("\tnvm write failed\n");
   }

}

/**
 *   @brief    Print ACS header
 *   @param    void
 *   @return   void
**/
static void val_host_print_acs_header(void)
{
   LOG(ALWAYS, "\n\n", 0, 0);
   LOG(ALWAYS,
   "***** RMM ACS Version %d.%d *****\n\n",
   ACS_MAJOR_VERSION,
   ACS_MINOR_VERSION);
}

/**
 *   @brief    Query test database and execute test from each suite one by one
 *   @param    primary_cpu_boot   -    Boolean value for primary cpu boot
 *   @return   void
**/
static void val_host_test_dispatch(bool primary_cpu_boot)
{
    uint32_t          test_result, i;
    uint32_t          reboot_run = 0;
    uint32_t          test_num_start = 0, test_num_end = 0;
    test_fptr_t       fn_ptr;
    val_test_info_ts       test_info = {0};
    val_regre_report_ts    regre_report = {0};

    if (primary_cpu_boot == true)
    {

        if (val_host_get_last_run_test_info(&test_info))
        {
           LOG(ERROR, "\tUnable to read last test_info\n", 0, 0);
           return;
        }

        if (test_info.test_num == VAL_INVALID_TEST_NUM)
        {
           val_host_print_acs_header();
           test_info.test_num = 1;
        } else
        {
           reboot_run = 1;
        }

        if (!reboot_run)
        {

#if defined(SUITE_TEST_RANGE)
            uint32_t          test_num, j;
            char *start_test_name = SUITE_TEST_RANGE_MIN;
            char *end_test_name = SUITE_TEST_RANGE_MAX;

            test_num = test_info.test_num;

            for (; test_num < total_tests - 1; test_num++)
            {
                if (val_strcmp((char *)test_list[test_num].test_name, start_test_name) == 0)
                {
                    test_num_start = test_num;
                }
            }

            test_num = test_info.test_num;
            for (; test_num < total_tests - 1; test_num++)
            {
                if (val_strcmp((char *)test_list[test_num].test_name, end_test_name) == 0)
                {
                    test_num_end = test_num;
                }
            }

            if (test_num_start > test_num_end)
            {
                j = test_num_start;
                test_num_start = test_num_end;
                test_num_end = j;
            }

            if ((val_nvm_write(VAL_NVM_OFFSET(NVM_END_TEST_NUM_INDEX),
                                                    &test_num_end, sizeof(test_num_end))))
            {
                        LOG(ERROR, "\tUnable to write nvm\n", 0, 0);
                        return;
            }
#else
            test_num_start = test_info.test_num;
            test_num_end = total_tests;

            if ((val_nvm_write(VAL_NVM_OFFSET(NVM_END_TEST_NUM_INDEX),
                                                    &test_num_end, sizeof(test_num_end))))
            {
                        LOG(ERROR, "\tUnable to write nvm\n", 0, 0);
                        return;
            }
#endif

        }
        else{
            test_num_start = test_info.test_num;
            test_num_end = test_info.end_test_num;
        }

        /* Iterate over test_list[] to run test one by one */
        for (i = test_num_start ; i <= test_num_end; i++)
        {

            fn_ptr = (test_fptr_t)(test_list[i].host_fn);

            if (fn_ptr == NULL)
                break;

            if (reboot_run)
            {
                /* Reboot case, find out whether reboot expected or not? */
                if (test_info.test_progress == TEST_REBOOTING)
                {
                    /* Reboot expected, declare previous test as pass */
                    val_set_status(RESULT_PASS(VAL_SUCCESS));
                } else
                {
                    /* Reboot not expected, declare previous test as error */
                    val_set_status(RESULT_ERROR(VAL_SIM_ERROR));
                }
                reboot_run = 0;
            } else {
                if ((val_nvm_write(VAL_NVM_OFFSET(NVM_CUR_TEST_NUM_INDEX),
                                            &i, sizeof(i))))
                {
                    LOG(ERROR, "\tUnable to write nvm\n", 0, 0);
                    return;
                }

                val_host_test_init(i);

                *(uint64_t *)(val_get_shared_region_base() + PRINT_OFFSET) = 0xffffffffffffffff;
	            /* Execute host test */
                skip_for_val_logs = 1;
                fn_ptr();
                skip_for_val_logs = 0;

	            val_host_test_exit();
            }

            test_result = val_host_report_status(i);

            if (val_nvm_read(VAL_NVM_OFFSET(NVM_TOTAL_PASS_INDEX),
                     &regre_report.total_pass, sizeof(uint32_t)) ||
                val_nvm_read(VAL_NVM_OFFSET(NVM_TOTAL_FAIL_INDEX),
                     &regre_report.total_fail, sizeof(uint32_t))  ||
                val_nvm_read(VAL_NVM_OFFSET(NVM_TOTAL_SKIP_INDEX),
                     &regre_report.total_skip, sizeof(uint32_t))  ||
                val_nvm_read(VAL_NVM_OFFSET(NVM_TOTAL_ERROR_INDEX),
                     &regre_report.total_error, sizeof(uint32_t)))

            {
                LOG(ERROR, "\tUnable to read regre_report\n", 0, 0);
                return;
            }

            switch (test_result)
            {
                case TEST_PASS:
                    regre_report.total_pass++;
                    break;
                case TEST_FAIL:
                    regre_report.total_fail++;
                    break;
                case TEST_SKIP:
                    regre_report.total_skip++;
                    break;
                case TEST_ERROR:
                    regre_report.total_error++;
                    break;
            }

            if (val_nvm_write(VAL_NVM_OFFSET(NVM_TOTAL_PASS_INDEX),
                     &regre_report.total_pass, sizeof(uint32_t)) ||
                val_nvm_write(VAL_NVM_OFFSET(NVM_TOTAL_FAIL_INDEX),
                     &regre_report.total_fail, sizeof(uint32_t))  ||
                val_nvm_write(VAL_NVM_OFFSET(NVM_TOTAL_SKIP_INDEX),
                     &regre_report.total_skip, sizeof(uint32_t))  ||
                val_nvm_write(VAL_NVM_OFFSET(NVM_TOTAL_ERROR_INDEX),
                     &regre_report.total_error, sizeof(uint32_t)))
            {
                LOG(ERROR, "\tUnable to write regre_report\n", 0, 0);
                return;
            }
        }

        /* Print Regression report */
        LOG(ALWAYS, "\n\n", 0, 0);
        LOG(ALWAYS, "REGRESSION REPORT: \n", 0, 0);
        LOG(ALWAYS, "==================\n", 0, 0);
        LOG(ALWAYS, "   TOTAL TESTS     : %d\n",
            (uint64_t)(regre_report.total_pass
            + regre_report.total_fail
            + regre_report.total_skip
            + regre_report.total_error),
            0);
        LOG(ALWAYS, "   TOTAL PASSED    : %d\n", regre_report.total_pass, 0);
        LOG(ALWAYS, "   TOTAL FAILED    : %d\n", regre_report.total_fail, 0);
        LOG(ALWAYS, "   TOTAL SKIPPED   : %d\n", regre_report.total_skip, 0);
        LOG(ALWAYS, "   TOTAL SIM ERROR : %d\n\n", regre_report.total_error, 0);
        LOG(ALWAYS, "******* END OF ACS *******\n", 0, 0);
    } else {
        /* Resume the current test for secondary cpu */
        fn_ptr = (test_fptr_t)(test_list[val_get_curr_test_num()].host_fn);
        if (fn_ptr == NULL)
        {
            VAL_PANIC("Invalid host test address for secondary cpu\n");
        }

        /* Execute host test fn for sec cpu */
        fn_ptr();
    }
}

/**
 *   @brief    C entry function for endpoint
 *   @param    primary_cpu_boot     -   Boolean value for primary cpu boot
 *   @return   void (Never returns)
**/
void val_host_main(bool primary_cpu_boot)
{
    val_set_security_state_flag(1);

    xlat_ctx_t  *host_xlat_ctx = val_host_get_xlat_ctx();

    if (primary_cpu_boot == true)
    {
        /* Add host region into TT data structure */
        val_host_add_mmap();

        /* Write page tables */
        val_setup_mmu(host_xlat_ctx);

        val_irq_setup();

        /* Physical IRQ interrupts are taken to EL2, unless they are routed to EL3.*/
        write_hcr_el2((1 << 27)); //TGE=1
    }

    /* Enable Stage-1 MMU */
    val_enable_mmu(host_xlat_ctx);

    /* Ready to run test regression */
    val_host_test_dispatch(primary_cpu_boot);

    LOG(ALWAYS, "HOST : Entering standby.. \n", 0, 0);
    pal_terminate_simulation();
}

