/*
 * Copyright (c) 2023-2025, Arm Limited or its affiliates. All rights reserved.
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
    return val_printf(*(print_verbosity_t *)(val_get_shared_region_base() + \
                        REALM_PRINTF_VERBOSITY_OFFSET),
                (char *)(val_get_shared_region_base() + REALM_PRINTF_MSG_OFFSET));
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
 *   @brief    This function notifies the framework about test
 *             intension of rebooting the platform. Test returns
 *             to framework on reset.
 *   @param    void
 *   @return   void
**/
void val_host_set_reboot_flag(void)
{
   uint32_t test_progress  = TEST_REBOOTING;

   LOG(INFO, "Setting reboot flag\n");
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
uint32_t val_host_get_last_run_test_info(test_info_t *test_info)
{
    uint32_t        reboot_run = 0;
    uint8_t         test_progress_pattern[] = {TEST_START, TEST_END, TEST_FAIL, TEST_REBOOTING};
    regre_report_t  regre_report = {0};

    if (val_nvm_read(VAL_NVM_OFFSET(NVM_CUR_TEST_NUM_INDEX),
            &test_info->test_num, sizeof(uint32_t)))
        return VAL_ERROR;

    if (val_nvm_read(VAL_NVM_OFFSET(NVM_END_TEST_NUM_INDEX),
            &test_info->end_test_num, sizeof(uint32_t)))
        return VAL_ERROR;

    if (val_nvm_read(VAL_NVM_OFFSET(NVM_TEST_PROGRESS_INDEX),
            &test_info->test_progress, sizeof(uint32_t)))
        return VAL_ERROR;

    val_log_test_info(test_info);

    /* Is power on reset or warm reset? Determine based on NVM content */
    reboot_run = is_reboot_run(test_info->test_progress, test_progress_pattern,
                           sizeof(test_progress_pattern)/sizeof(test_progress_pattern[0]));

    /* Power on reset : Initiliase necessary data structure
     * Warm reset : Return previously executed test number
     * */
    if (!reboot_run)
    {
         val_reset_test_info_fields(test_info);

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

    val_log_final_test_status(test_info, &regre_report);
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
   LOG(DBG, "test_num=%d\n", val_get_curr_test_num());

   val_strcat(testname,
                (char *)test_list[test_num].suite_name,
                sizeof(testname));
   val_strcat(testname,
                (char *)test_list[test_num].test_name,
                sizeof(testname));
   val_strcat(testname, "\n", sizeof(testname));

   LOG(ALWAYS, testname);

   if (val_nvm_write(VAL_NVM_OFFSET(NVM_TEST_PROGRESS_INDEX),
           &test_progress, sizeof(uint32_t)))
   {
      VAL_PANIC("\tnvm write failed\n");
   }

   if (val_watchdog_enable())
   {
      VAL_PANIC("\tWatchdog enable failed\n");
   }

   /* Reset mem_track structure incase postamble is skipped */
   val_host_reset_mem_tack();

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
         LOG(ERROR, "val_host_postamble failed\n");
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
   LOG(ALWAYS, "\n\n");
#if defined(RMM_V_1_1)
   LOG(ALWAYS, "********* RMM v1.1 ACS **********\n\n")
   LOG(ALWAYS, " 1. Planes - BETA \n\n")
   LOG(ALWAYS, " 2. MEC and LFA - BETA \n\n")
   LOG(ALWAYS, "*********************************\n\n")
#elif defined(RMM_V_1_0)
   LOG(ALWAYS, "********* RMM v1.0 ACS EAC **********\n\n")
#endif
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
    uint32_t          feature_planes_supported = 0, feature_mec_supported = 0;
    test_fptr_t       fn_ptr;
    test_info_t       test_info = {0};
    regre_report_t    regre_report = {0};

    if (primary_cpu_boot == true)
    {

        if (val_host_get_last_run_test_info(&test_info))
        {
           LOG(ERROR, "Unable to read last test_info\n");
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
            uint32_t          test_num;
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

            val_sort_indices(&test_num_start, &test_num_end);
            if ((val_nvm_write(VAL_NVM_OFFSET(NVM_END_TEST_NUM_INDEX),
                                                    &test_num_end, sizeof(test_num_end))))
            {
                        LOG(ERROR, "Unable to write nvm\n");
                        return;
            }
#else
            test_num_start = test_info.test_num;
            test_num_end = total_tests;

            if ((val_nvm_write(VAL_NVM_OFFSET(NVM_END_TEST_NUM_INDEX),
                                                    &test_num_end, sizeof(test_num_end))))
            {
                        LOG(ERROR, "Unable to write nvm\n");
                        return;
            }
#endif

        }
        else{
            test_num_start = test_info.test_num;
            test_num_end = test_info.end_test_num;
        }

#ifdef RMM_V_1_1
        if (!val_strcmp(SUITE, "all") || !val_strcmp(SUITE, "planes"))
        {
            feature_planes_supported = val_host_rmm_supports_planes();
            if (!feature_planes_supported)
            {
                LOG(ALWAYS, "Planes feature not supported, Skipping planes Suite\n\n", 0, 0);
                LOG(ALWAYS, "*********************************\n", 0, 0)
            }
        }

        if (!val_strcmp(SUITE, "all") || !val_strcmp(SUITE, "mec"))
        {
            feature_mec_supported = val_host_rmm_supports_mec();
            if (!feature_mec_supported)
            {
                LOG(ALWAYS, "MEC feature not supported, Skipping mec Suite\n\n", 0, 0);
                LOG(ALWAYS, "*********************************\n", 0, 0)
            }
        }
#endif
        /* Iterate over test_list[] to run test one by one */
        for (i = test_num_start ; i <= test_num_end; i++)
        {

            fn_ptr = (test_fptr_t)(test_list[i].host_fn);

            if (fn_ptr == NULL)
                break;

            /* Skip if RMM do not support planes */
            if ((!feature_planes_supported) && (!val_strcmp((char *)test_list[i].sub_suite_name,
                                                                                     "planes")))
                continue;

            /* Skip if RMM do not support planes */
            if ((!feature_mec_supported) && (!val_strcmp((char *)test_list[i].sub_suite_name,
                                                                                     "mec")))
                    continue;

            if (reboot_run)
            {
                /* Reboot case, find out whether reboot expected or not? */
                val_handle_reboot_result(test_info.test_progress);
                reboot_run = 0;
            } else {
                if ((val_nvm_write(VAL_NVM_OFFSET(NVM_CUR_TEST_NUM_INDEX),
                                            &i, sizeof(i))))
                {
                    LOG(ERROR, "Unable to write nvm\n");
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

            test_result = val_report_status();

            if (val_nvm_read(VAL_NVM_OFFSET(NVM_TOTAL_PASS_INDEX),
                     &regre_report.total_pass, sizeof(uint32_t)) ||
                val_nvm_read(VAL_NVM_OFFSET(NVM_TOTAL_FAIL_INDEX),
                     &regre_report.total_fail, sizeof(uint32_t))  ||
                val_nvm_read(VAL_NVM_OFFSET(NVM_TOTAL_SKIP_INDEX),
                     &regre_report.total_skip, sizeof(uint32_t))  ||
                val_nvm_read(VAL_NVM_OFFSET(NVM_TOTAL_ERROR_INDEX),
                     &regre_report.total_error, sizeof(uint32_t)))

            {
                LOG(ERROR, "Unable to read regre_report\n");
                return;
            }

            val_update_regression_report(test_result, &regre_report);

            if (val_nvm_write(VAL_NVM_OFFSET(NVM_TOTAL_PASS_INDEX),
                     &regre_report.total_pass, sizeof(uint32_t)) ||
                val_nvm_write(VAL_NVM_OFFSET(NVM_TOTAL_FAIL_INDEX),
                     &regre_report.total_fail, sizeof(uint32_t))  ||
                val_nvm_write(VAL_NVM_OFFSET(NVM_TOTAL_SKIP_INDEX),
                     &regre_report.total_skip, sizeof(uint32_t))  ||
                val_nvm_write(VAL_NVM_OFFSET(NVM_TOTAL_ERROR_INDEX),
                     &regre_report.total_error, sizeof(uint32_t)))
            {
                LOG(ERROR, "Unable to write regre_report\n");
                return;
            }
        }

        /* Print Regression report */
        val_print_regression_report(&regre_report);
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
    val_set_security_state_flag(SEC_STATE_NS);

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

    LOG(ALWAYS, "HOST : Entering standby.. \n");
    pal_terminate_simulation();
}

