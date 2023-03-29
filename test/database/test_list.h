/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/* Declaration of test funcs prototype */
#ifndef TEST_FUNC_DATABASE

/*command testcase declaration starts here*/
DECLARE_TEST_FN(cmd_granule_delegate);
DECLARE_TEST_FN(cmd_granule_undelegate);
DECLARE_TEST_FN(cmd_realm_create);
DECLARE_TEST_FN(cmd_realm_activate);
DECLARE_TEST_FN(cmd_rtt_read_entry);
DECLARE_TEST_FN(cmd_rtt_init_ripas);
DECLARE_TEST_FN(cmd_rtt_create);
DECLARE_TEST_FN(cmd_data_create);
DECLARE_TEST_FN(cmd_data_create_unknown);
DECLARE_TEST_FN(cmd_rtt_map_unprotected);
DECLARE_TEST_FN(cmd_rec_create);
DECLARE_TEST_FN(cmd_rec_enter);
DECLARE_TEST_FN(cmd_rmi_version);
DECLARE_TEST_FN(cmd_rsi_version);
DECLARE_TEST_FN(cmd_realm_config);
DECLARE_TEST_FN(cmd_ipa_state_get);
DECLARE_TEST_FN(cmd_ipa_state_set);
/*command testcase declaration ends here*/

/*val sanity testcase starts here*/
DECLARE_TEST_FN(cmd_multithread_realm_up);
DECLARE_TEST_FN(cmd_multithread_realm_mp);
DECLARE_TEST_FN(cmd_secure_test);
/*val sanity testcase ends here*/

/*memory management testcase declaration starts here*/
DECLARE_TEST_FN(mm_ripas_change);
DECLARE_TEST_FN(mm_ripas_change_reject);
DECLARE_TEST_FN(mm_ripas_change_partial);
DECLARE_TEST_FN(mm_hipas_assigned_ripas_empty_da);
DECLARE_TEST_FN(mm_hipas_destroyed_ripas_ram_da);
DECLARE_TEST_FN(mm_hipas_assigned_ripas_empty_ia);
DECLARE_TEST_FN(mm_hipas_assigned_ia);
DECLARE_TEST_FN(mm_hipas_assigned_da);
DECLARE_TEST_FN(mm_unprotected_ipa_boundary);
DECLARE_TEST_FN(mm_protected_ipa_boundary);
DECLARE_TEST_FN(mm_gpf_exception);
DECLARE_TEST_FN(mm_rtt_translation_table);
DECLARE_TEST_FN(mm_rtt_fold_assigned);
DECLARE_TEST_FN(mm_rtt_fold_unassigned);
DECLARE_TEST_FN(mm_rtt_fold_u_ipa_assigned);
DECLARE_TEST_FN(mm_rtt_fold_destroyed);
DECLARE_TEST_FN(mm_hipas_destroyed_ripas_ram_ia);
DECLARE_TEST_FN(mm_hipas_unassigned_ripas_empty_da);
DECLARE_TEST_FN(mm_hipas_unassigned_ripas_empty_ia);
DECLARE_TEST_FN(mm_hipas_destroyed_ripas_empty_da);
DECLARE_TEST_FN(mm_hipas_destroyed_ripas_empty_ia);
DECLARE_TEST_FN(mm_hipas_unassigned_ripas_ram_da);
DECLARE_TEST_FN(mm_hipas_unassigned_ripas_ram_ia);
DECLARE_TEST_FN(mm_feat_s2fwb_check_1);
DECLARE_TEST_FN(mm_rtt_level_start);
DECLARE_TEST_FN(mm_feat_s2fwb_check_2);
DECLARE_TEST_FN(mm_feat_s2fwb_check_3);
/*memory management testcase declaration ends here*/

/*Exception model declaration starts here*/
DECLARE_TEST_FN(exception_rec_exit_wfe);
DECLARE_TEST_FN(exception_rec_exit_wfi);
DECLARE_TEST_FN(exception_rec_exit_hostcall);
DECLARE_TEST_FN(exception_rec_exit_psci);
DECLARE_TEST_FN(exception_realm_unsupported_smc);
DECLARE_TEST_FN(exception_rec_exit_hvc);
DECLARE_TEST_FN(exception_rec_exit_ripas);
DECLARE_TEST_FN(exception_rec_exit_ia);
DECLARE_TEST_FN(exception_emulatable_da);
DECLARE_TEST_FN(exception_non_emulatable_da);
DECLARE_TEST_FN(exception_non_emulatable_da_1);
DECLARE_TEST_FN(exception_non_emulatable_da_2);
/*Exception model declaration ends here*/

/*GIC testcase declaration starts here*/
DECLARE_TEST_FN(gic_hcr_invalid);
DECLARE_TEST_FN(gic_ctrl_list_invalid);
/*GIC testcase declaration ends here*/

#else /* TEST_FUNC_DATABASE */
/* Add test funcs to the respective host/realm/secure test_list array */
#if (defined(d_all) || defined(d_command))
    #if (defined(TEST_COMBINE) || defined(d_cmd_granule_delegate))
    HOST_TEST(command, cmd_granule_delegate),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_granule_undelegate))
    HOST_TEST(command, cmd_granule_undelegate),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_realm_create))
    HOST_TEST(command, cmd_realm_create),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_realm_activate))
    HOST_REALM_TEST(command, cmd_realm_activate),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_rtt_read_entry))
    HOST_TEST(command, cmd_rtt_read_entry),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_rtt_init_ripas))
    HOST_REALM_TEST(command, cmd_rtt_init_ripas),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_rtt_create))
    HOST_TEST(command, cmd_rtt_create),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_data_create))
    HOST_REALM_TEST(command, cmd_data_create),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_rtt_map_unprotected))
    HOST_TEST(command, cmd_rtt_map_unprotected),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_rec_create))
    HOST_REALM_TEST(command, cmd_rec_create),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_rec_enter))
    HOST_REALM_TEST(command, cmd_rec_enter),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_rmi_version))
    HOST_TEST(command, cmd_rmi_version),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_multithread_realm_up))
    HOST_REALM_TEST(command, cmd_multithread_realm_up),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_multithread_realm_mp))
    HOST_REALM_TEST(command, cmd_multithread_realm_mp),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_rsi_version))
    HOST_REALM_TEST(command, cmd_rsi_version),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_data_create_unknown))
    HOST_TEST(command, cmd_data_create_unknown),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_secure_test))
    HOST_SECURE_TEST(command, cmd_secure_test),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_realm_config))
    HOST_REALM_TEST(command, cmd_realm_config),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_ipa_state_get))
    HOST_REALM_TEST(command, cmd_ipa_state_get),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_ipa_state_set))
    HOST_REALM_TEST(command, cmd_ipa_state_set),
    #endif

#endif /* (SUITE == all || SUITE == command) */

#if (defined(d_all) || defined(d_exception))
    #if (defined(TEST_COMBINE) || defined(d_exception_rec_exit_wfe))
        HOST_REALM_TEST(exception, exception_rec_exit_wfe),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_exception_rec_exit_wfi))
        HOST_REALM_TEST(exception, exception_rec_exit_wfi),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_exception_rec_exit_hostcall))
        HOST_REALM_TEST(exception, exception_rec_exit_hostcall),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_exception_rec_exit_psci))
        HOST_REALM_TEST(exception, exception_rec_exit_psci),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_exception_realm_unsupported_smc))
        HOST_REALM_TEST(exception, exception_realm_unsupported_smc),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_exception_rec_exit_hvc))
        HOST_REALM_TEST(exception, exception_rec_exit_hvc),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_exception_rec_exit_ripas))
        HOST_REALM_TEST(exception, exception_rec_exit_ripas),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_exception_rec_exit_ia))
        HOST_REALM_TEST(exception, exception_rec_exit_ia),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_exception_emulatable_da))
        HOST_REALM_TEST(exception, exception_emulatable_da),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_exception_non_emulatable_da))
        HOST_REALM_TEST(exception, exception_non_emulatable_da),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_exception_non_emulatable_da_1))
        HOST_REALM_TEST(exception, exception_non_emulatable_da_1),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_exception_non_emulatable_da_2))
        HOST_REALM_TEST(exception, exception_non_emulatable_da_2),
    #endif

#endif /* #if (defined(d_all) || defined(d_exception)) */

#if (defined(d_all) || defined(d_gic))
    #if (defined(TEST_COMBINE) || defined(d_gic_hcr_invalid))
    HOST_REALM_TEST(gic, gic_hcr_invalid),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_gic_ctrl_list_invalid))
    HOST_REALM_TEST(gic, gic_ctrl_list_invalid),
    #endif
#endif /* #if (defined(d_all) || defined(d_gic)) */

#if (defined(d_all) || defined(d_memory_management))
    #if (defined(TEST_COMBINE) || defined(d_mm_ripas_change))
    HOST_REALM_TEST(memory_management, mm_ripas_change),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_ripas_change_reject))
    HOST_REALM_TEST(memory_management, mm_ripas_change_reject),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_ripas_change_partial))
    HOST_REALM_TEST(memory_management, mm_ripas_change_partial),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_hipas_assigned_ripas_empty_da))
    HOST_REALM_TEST(memory_management, mm_hipas_assigned_ripas_empty_da),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_hipas_destroyed_ripas_ram_da))
    HOST_REALM_TEST(memory_management, mm_hipas_destroyed_ripas_ram_da),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_hipas_assigned_ripas_empty_ia))
    HOST_REALM_TEST(memory_management, mm_hipas_assigned_ripas_empty_ia),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_hipas_assigned_ia))
    HOST_REALM_TEST(memory_management, mm_hipas_assigned_ia),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_hipas_assigned_da))
    HOST_REALM_TEST(memory_management, mm_hipas_assigned_da),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_gpf_exception))
    HOST_TEST(memory_management, mm_gpf_exception),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_unprotected_ipa_boundary))
    HOST_REALM_TEST(memory_management, mm_unprotected_ipa_boundary),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_protected_ipa_boundary))
    HOST_REALM_TEST(memory_management, mm_protected_ipa_boundary),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_rtt_translation_table))
    HOST_REALM_TEST(memory_management, mm_rtt_translation_table),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_hipas_destroyed_ripas_ram_ia))
    HOST_REALM_TEST(memory_management, mm_hipas_destroyed_ripas_ram_ia),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_hipas_unassigned_ripas_empty_da))
    HOST_REALM_TEST(memory_management, mm_hipas_unassigned_ripas_empty_da),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_hipas_unassigned_ripas_empty_ia))
    HOST_REALM_TEST(memory_management, mm_hipas_unassigned_ripas_empty_ia),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_hipas_destroyed_ripas_empty_da))
    HOST_REALM_TEST(memory_management, mm_hipas_destroyed_ripas_empty_da),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_hipas_destroyed_ripas_empty_ia))
    HOST_REALM_TEST(memory_management, mm_hipas_destroyed_ripas_empty_ia),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_hipas_unassigned_ripas_ram_da))
    HOST_REALM_TEST(memory_management, mm_hipas_unassigned_ripas_ram_da),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_hipas_unassigned_ripas_ram_ia))
    HOST_REALM_TEST(memory_management, mm_hipas_unassigned_ripas_ram_ia),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_feat_s2fwb_check_1))
    HOST_REALM_TEST(memory_management, mm_feat_s2fwb_check_1),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_rtt_level_start))
    HOST_REALM_TEST(memory_management, mm_rtt_level_start),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_rtt_fold_assigned))
    HOST_REALM_TEST(memory_management, mm_rtt_fold_assigned),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_rtt_fold_unassigned))
    HOST_REALM_TEST(memory_management, mm_rtt_fold_unassigned),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_rtt_fold_destroyed))
    HOST_REALM_TEST(memory_management, mm_rtt_fold_destroyed),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_rtt_fold_u_ipa_assigned))
    HOST_REALM_TEST(memory_management, mm_rtt_fold_u_ipa_assigned),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_feat_s2fwb_check_2))
    HOST_REALM_TEST(memory_management, mm_feat_s2fwb_check_2),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_feat_s2fwb_check_3))
    HOST_REALM_TEST(memory_management, mm_feat_s2fwb_check_3),
    #endif

#endif /* #if (defined(d_all) || defined(d_memory_management)) */

#endif /* TEST_FUNC_DATABASE */
