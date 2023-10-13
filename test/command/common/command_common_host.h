 /*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#ifndef _CMD_COMMON_HOST_H
#define _CMD_COMMON_HOST_H
#include "val_host_command.h"

uint32_t val_host_realm_create_common(val_host_realm_ts *realm);
uint32_t val_host_rec_create_common(val_host_realm_ts *realm, val_host_rec_params_ts *params);
uint64_t g_delegated_prep_sequence(void);
uint64_t g_undelegated_prep_sequence(void);
uint64_t g_unaligned_prep_sequence(uint64_t gran);
uint64_t g_dev_mem_prep_sequence(void);
uint64_t g_outside_of_permitted_pa_prep_sequence(void);
uint64_t g_secure_prep_sequence(void);
uint64_t g_data_prep_sequence(uint64_t rd, uint64_t ipa);
uint64_t ipa_outside_of_permitted_ipa_prep_sequence(void);
uint64_t ipa_protected_unmapped_prep_sequence(void);
uint64_t ipa_protected_assigned_ram_prep_sequence(uint64_t rd);
uint64_t ipa_protected_assigned_empty_prep_sequence(uint64_t rd);
uint64_t ipa_protected_destroyed_prep_sequence(uint64_t rd);
uint64_t ipa_unprotected_unmapped_prep_sequence(void);
uint64_t ipa_unprotected_assinged_prep_sequence(uint64_t rd);
uint64_t ipa_unprotected_unassigned_prep_sequence(uint64_t rd);
uint64_t ipa_protected_unassigned_ram_prep_sequence(uint64_t rd);
uint64_t ipa_protected_unassigned_empty_prep_sequence(uint64_t rd);

#endif /* _CMD_COMMON_HOST_H */
