/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/* Declaration of test funcs prototype */
#ifndef TEST_FUNC_DATABASE

/*command testcase declaration starts here*/
DECLARE_TEST_FN(cmd_rmi_version);
DECLARE_TEST_FN(cmd_granule_delegate);
DECLARE_TEST_FN(cmd_granule_undelegate);
DECLARE_TEST_FN(cmd_realm_create);
DECLARE_TEST_FN(cmd_rsi_version);
DECLARE_TEST_FN(cmd_realm_activate);
DECLARE_TEST_FN(cmd_rtt_read_entry);
DECLARE_TEST_FN(cmd_rtt_init_ripas);
DECLARE_TEST_FN(cmd_rtt_create);
DECLARE_TEST_FN(cmd_data_create);
DECLARE_TEST_FN(cmd_data_create_unknown);
DECLARE_TEST_FN(cmd_rtt_map_unprotected);
DECLARE_TEST_FN(cmd_rec_create);
DECLARE_TEST_FN(cmd_rec_enter);
DECLARE_TEST_FN(cmd_realm_config);
DECLARE_TEST_FN(cmd_ipa_state_get);
DECLARE_TEST_FN(cmd_ipa_state_set);
DECLARE_TEST_FN(cmd_psci_complete);
DECLARE_TEST_FN(cmd_rmi_features);
DECLARE_TEST_FN(cmd_rec_aux_count);
DECLARE_TEST_FN(cmd_data_destroy);
DECLARE_TEST_FN(cmd_realm_destroy);
DECLARE_TEST_FN(cmd_rec_destroy);
DECLARE_TEST_FN(cmd_rtt_destroy);
DECLARE_TEST_FN(cmd_rtt_unmap_unprotected);
DECLARE_TEST_FN(cmd_psci_version);
DECLARE_TEST_FN(cmd_psci_features);
DECLARE_TEST_FN(cmd_cpu_off);
DECLARE_TEST_FN(cmd_cpu_suspend);
DECLARE_TEST_FN(cmd_system_off);
DECLARE_TEST_FN(cmd_system_reset);
DECLARE_TEST_FN(cmd_affinity_info);
DECLARE_TEST_FN(cmd_cpu_on);
DECLARE_TEST_FN(cmd_measurement_read);
DECLARE_TEST_FN(cmd_measurement_extend);
DECLARE_TEST_FN(cmd_attestation_token_init);
DECLARE_TEST_FN(cmd_attestation_token_continue);
DECLARE_TEST_FN(cmd_host_call);
DECLARE_TEST_FN(cmd_rtt_set_ripas);
DECLARE_TEST_FN(cmd_rtt_fold);
DECLARE_TEST_FN(cmd_rsi_features);
/*command testcase declaration ends here*/

/*val sanity testcase starts here*/
DECLARE_TEST_FN(cmd_multithread_realm_up);
DECLARE_TEST_FN(cmd_multithread_realm_mp);
DECLARE_TEST_FN(cmd_secure_test);
/*val sanity testcase ends here*/

/*ATTESTATION and MEASUREMENT testcase declaration starts here*/
DECLARE_TEST_FN(measurement_immutable_rim);
DECLARE_TEST_FN(measurement_initial_rem_is_zero);
DECLARE_TEST_FN(measurement_rim_order);
DECLARE_TEST_FN(attestation_token_verify)
DECLARE_TEST_FN(attestation_rpv_value);
DECLARE_TEST_FN(attestation_challenge_data_verification);
DECLARE_TEST_FN(attestation_token_init);
DECLARE_TEST_FN(attestation_realm_measurement_type);
DECLARE_TEST_FN(attestation_platform_challenge_size);
DECLARE_TEST_FN(attestation_rem_extend_check);
DECLARE_TEST_FN(attestation_rem_extend_check_realm_token);
DECLARE_TEST_FN(attestation_rec_exit_irq);

/*ATTESTATION and MEASUREMENT testcase declaration ends here*/

/*memory management testcase declaration starts here*/
DECLARE_TEST_FN(mm_ripas_change);
DECLARE_TEST_FN(mm_ripas_change_reject);
DECLARE_TEST_FN(mm_ripas_change_partial);
DECLARE_TEST_FN(mm_hipas_assigned_ripas_empty_da_ia);
DECLARE_TEST_FN(mm_hipas_unassigned_ns_da_ia);
DECLARE_TEST_FN(mm_unprotected_ipa_boundary);
DECLARE_TEST_FN(mm_protected_ipa_boundary);
DECLARE_TEST_FN(mm_gpf_exception);
DECLARE_TEST_FN(mm_rtt_translation_table);
DECLARE_TEST_FN(mm_rtt_fold_assigned);
DECLARE_TEST_FN(mm_rtt_fold_unassigned);
DECLARE_TEST_FN(mm_rtt_fold_unassigned_ns);
DECLARE_TEST_FN(mm_rtt_fold_assigned_ns);
DECLARE_TEST_FN(mm_ripas_destroyed_da_ia);
DECLARE_TEST_FN(mm_hipas_unassigned_ripas_empty_da_ia);
DECLARE_TEST_FN(mm_hipas_unassigned_ripas_ram_da_ia);
DECLARE_TEST_FN(mm_feat_s2fwb_check_1);
DECLARE_TEST_FN(mm_rtt_level_start);
DECLARE_TEST_FN(mm_feat_s2fwb_check_2);
DECLARE_TEST_FN(mm_feat_s2fwb_check_3);
DECLARE_TEST_FN(mm_ha_hd_access);
DECLARE_TEST_FN(mm_realm_access_outside_ipa);
/*memory management testcase declaration ends here*/

/*Exception model declaration starts here*/
DECLARE_TEST_FN(exception_rec_exit_wfe);
DECLARE_TEST_FN(exception_rec_exit_wfi);
DECLARE_TEST_FN(exception_rec_exit_irq);
DECLARE_TEST_FN(exception_rec_exit_fiq);
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
DECLARE_TEST_FN(gic_ctrl_list);
DECLARE_TEST_FN(gic_obsv_vmcr);
DECLARE_TEST_FN(gic_timer_val_read);
DECLARE_TEST_FN(gic_timer_rel1_trig);
DECLARE_TEST_FN(gic_timer_nsel2_trig);
DECLARE_TEST_FN(gic_ctrl_hcr);
/*GIC testcase declaration ends here*/

/*PMU and DEBUG testcase declaration starts here*/
DECLARE_TEST_FN(pmu_overflow);
/*PMU and DEBUG testcase declaration ends here*/

#else /* TEST_FUNC_DATABASE */
/* Add test funcs to the respective host/realm/secure test_list array */
#if (defined(d_all) || defined(d_command))
    #if (defined(TEST_COMBINE) || defined(d_cmd_rmi_version))
    HOST_TEST(command, cmd_rmi_version),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_granule_delegate))
    HOST_TEST(command, cmd_granule_delegate),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_granule_undelegate))
    HOST_TEST(command, cmd_granule_undelegate),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_realm_create))
    HOST_TEST(command, cmd_realm_create),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_rsi_version))
    HOST_REALM_TEST(command, cmd_rsi_version),
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
    #if (defined(TEST_COMBINE) || defined(d_cmd_multithread_realm_up))
    HOST_REALM_TEST(command, cmd_multithread_realm_up),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_multithread_realm_mp))
    HOST_REALM_TEST(command, cmd_multithread_realm_mp),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_rsi_features))
    HOST_REALM_TEST(command, cmd_rsi_features),
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
    #if (defined(TEST_COMBINE) || defined(d_cmd_psci_complete))
    HOST_REALM_TEST(command, cmd_psci_complete),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_rmi_features))
    HOST_TEST(command, cmd_rmi_features),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_rec_aux_count))
    HOST_TEST(command, cmd_rec_aux_count),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_data_destroy))
    HOST_TEST(command, cmd_data_destroy),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_realm_destroy))
    HOST_TEST(command, cmd_realm_destroy),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_rec_destroy))
    HOST_TEST(command, cmd_rec_destroy),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_rtt_destroy))
    HOST_TEST(command, cmd_rtt_destroy),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_rtt_unmap_unprotected))
    HOST_TEST(command, cmd_rtt_unmap_unprotected),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_psci_version))
    HOST_REALM_TEST(command, cmd_psci_version),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_psci_features))
    HOST_REALM_TEST(command, cmd_psci_features),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_cpu_off))
    HOST_REALM_TEST(command, cmd_cpu_off),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_cpu_suspend))
    HOST_REALM_TEST(command, cmd_cpu_suspend),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_system_off))
    HOST_REALM_TEST(command, cmd_system_off),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_system_reset))
    HOST_REALM_TEST(command, cmd_system_reset),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_measurement_read))
    HOST_REALM_TEST(command, cmd_measurement_read),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_measurement_extend))
    HOST_REALM_TEST(command, cmd_measurement_extend),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_attestation_token_init))
    HOST_REALM_TEST(command, cmd_attestation_token_init),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_attestation_token_continue))
    HOST_REALM_TEST(command, cmd_attestation_token_continue),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_host_call))
    HOST_REALM_TEST(command, cmd_host_call),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_affinity_info))
    HOST_REALM_TEST(command, cmd_affinity_info),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_cpu_on))
    HOST_REALM_TEST(command, cmd_cpu_on),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_rtt_set_ripas))
    HOST_REALM_TEST(command, cmd_rtt_set_ripas),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_cmd_rtt_fold))
    HOST_TEST(command, cmd_rtt_fold),
    #endif

#endif /* (SUITE == all || SUITE == command) */

#if (defined(d_all) || defined(d_exception))
    #if (defined(TEST_COMBINE) || defined(d_exception_rec_exit_wfe))
        HOST_REALM_TEST(exception, exception_rec_exit_wfe),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_exception_rec_exit_wfi))
        HOST_REALM_TEST(exception, exception_rec_exit_wfi),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_exception_rec_exit_irq))
        HOST_REALM_TEST(exception, exception_rec_exit_irq),
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
    #if (defined(TEST_COMBINE) || defined(d_gic_timer_nsel2_trig))
    HOST_REALM_TEST(gic, gic_timer_nsel2_trig),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_gic_hcr_invalid))
    HOST_REALM_TEST(gic, gic_hcr_invalid),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_gic_ctrl_list_invalid))
    HOST_REALM_TEST(gic, gic_ctrl_list_invalid),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_gic_ctrl_list))
    HOST_REALM_TEST(gic, gic_ctrl_list),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_gic_obsv_vmcr))
    HOST_REALM_TEST(gic, gic_obsv_vmcr),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_gic_timer_val_read))
    HOST_REALM_TEST(gic, gic_timer_val_read),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_gic_timer_rel1_trig))
    HOST_REALM_TEST(gic, gic_timer_rel1_trig),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_gic_ctrl_hcr))
    HOST_REALM_TEST(gic, gic_ctrl_hcr),
    #endif
#endif /* #if (defined(d_all) || defined(d_gic)) */

#if (defined(d_all) || defined(d_pmu_debug))
    #if (defined(TEST_COMBINE) || defined(d_pmu_overflow))
    HOST_REALM_TEST(pmu_debug, pmu_overflow),
    #endif
#endif /* #if (defined(d_all) || defined(d_pmu_debug)) */

#if (defined(d_all) || defined(d_attestation_measurement))
    #if (defined(TEST_COMBINE) || defined(d_measurement_immutable_rim))
    HOST_REALM_TEST(attestation_measurement, measurement_immutable_rim),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_measurement_initial_rem_is_zero))
    HOST_REALM_TEST(attestation_measurement, measurement_initial_rem_is_zero),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_measurement_rim_order))
    HOST_REALM_TEST(attestation_measurement, measurement_rim_order),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_attestation_token_verify))
    HOST_REALM_TEST(attestation_measurement, attestation_token_verify),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_attestation_rpv_value))
    HOST_REALM_TEST(attestation_measurement, attestation_rpv_value),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_attestation_challenge_data_verification))
    HOST_REALM_TEST(attestation_measurement, attestation_challenge_data_verification),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_attestation_token_init))
    HOST_REALM_TEST(attestation_measurement, attestation_token_init),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_attestation_realm_measurement_type))
    HOST_REALM_TEST(attestation_measurement, attestation_realm_measurement_type),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_attestation_platform_challenge_size))
    HOST_REALM_TEST(attestation_measurement, attestation_platform_challenge_size),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_attestation_rem_extend_check))
    HOST_REALM_TEST(attestation_measurement, attestation_rem_extend_check),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_attestation_rem_extend_check_realm_token))
    HOST_REALM_TEST(attestation_measurement, attestation_rem_extend_check_realm_token),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_attestation_rec_exit_irq))
    HOST_REALM_TEST(attestation_measurement, attestation_rec_exit_irq),
    #endif
#endif /* #if (defined(d_all) || defined(d_attestation_measurement)) */

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
    #if (defined(TEST_COMBINE) || defined(d_mm_hipas_assigned_ripas_empty_da_ia))
    HOST_REALM_TEST(memory_management, mm_hipas_assigned_ripas_empty_da_ia),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_ripas_destroyed_da_ia))
    HOST_REALM_TEST(memory_management, mm_ripas_destroyed_da_ia),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_hipas_unassigned_ns_da_ia))
    HOST_REALM_TEST(memory_management, mm_hipas_unassigned_ns_da_ia),
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
    #if (defined(TEST_COMBINE) || defined(d_mm_hipas_unassigned_ripas_empty_da_ia))
    HOST_REALM_TEST(memory_management, mm_hipas_unassigned_ripas_empty_da_ia),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_hipas_unassigned_ripas_ram_da_ia))
    HOST_REALM_TEST(memory_management, mm_hipas_unassigned_ripas_ram_da_ia),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_feat_s2fwb_check_1))
    HOST_REALM_TEST(memory_management, mm_feat_s2fwb_check_1),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_feat_s2fwb_check_2))
    HOST_REALM_TEST(memory_management, mm_feat_s2fwb_check_2),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_feat_s2fwb_check_3))
    HOST_REALM_TEST(memory_management, mm_feat_s2fwb_check_3),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_rtt_level_start))
    HOST_REALM_TEST(memory_management, mm_rtt_level_start),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_ha_hd_access))
    HOST_REALM_TEST(memory_management, mm_ha_hd_access),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_realm_access_outside_ipa))
    HOST_REALM_TEST(memory_management, mm_realm_access_outside_ipa),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_rtt_fold_assigned))
    HOST_REALM_TEST(memory_management, mm_rtt_fold_assigned),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_rtt_fold_unassigned))
    HOST_REALM_TEST(memory_management, mm_rtt_fold_unassigned),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_rtt_fold_unassigned_ns))
    HOST_REALM_TEST(memory_management, mm_rtt_fold_unassigned_ns),
    #endif
    #if (defined(TEST_COMBINE) || defined(d_mm_rtt_fold_assigned_ns))
    HOST_REALM_TEST(memory_management, mm_rtt_fold_assigned_ns),
    #endif

#endif /* #if (defined(d_all) || defined(d_memory_management)) */

#endif /* TEST_FUNC_DATABASE */
