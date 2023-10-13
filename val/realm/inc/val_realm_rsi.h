/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_REALM_RSI_H_
#define _VAL_REALM_RSI_H_

#include "val.h"
#include "val_framework.h"
#include "val_mmu.h"
#include "val_smc.h"
#include "val_rmm.h"

#define RSI_HOST_CALL_NR_GPRS 31

#define SET_MEMBER_RSI	SET_MEMBER
typedef struct _val_realm_rsi_host_call {
    SET_MEMBER_RSI(struct {
        /* Immediate value */
        unsigned int imm;		/* Offset 0 */
        /* Registers */
        unsigned long gprs[RSI_HOST_CALL_NR_GPRS];
        }, 0, 0x100);
} val_realm_rsi_host_call_t;

typedef struct __attribute__((packed)) {
    uint8_t destroyed:1;
    uint64_t unused:63;
} val_realm_ripas_change_flags_ts;

typedef struct {
	/* IPA width in bits */
    SET_MEMBER_RSI(unsigned long ipa_width, 0, 8);		/* Offset 0 */
	/* Hash algorithm */
    SET_MEMBER_RSI(unsigned long hash_algo, 8, 0x1000);	/* Offset 8 */
} val_realm_rsi_realm_config_ts;

uint64_t val_realm_rsi_version(void);
uint64_t val_realm_rsi_realm_config(uint64_t buff);
uint64_t val_realm_rsi_host_call(uint16_t imm);
val_realm_rsi_host_call_t *val_realm_rsi_host_call_ripas(uint16_t imm);
uint64_t val_realm_rsi_host_call_struct(uint64_t gv_realm_host_call1);
uint64_t val_realm_get_ipa_width(void);
val_smc_param_ts val_realm_rsi_ipa_state_set(uint64_t base, uint64_t size, uint8_t ripas,
                                                                         uint64_t flags);
val_smc_param_ts val_realm_rsi_ipa_state_get(uint64_t ipa_base);
uint64_t val_realm_rsi_host_params(val_realm_rsi_host_call_t *realm_host_params);
val_smc_param_ts val_realm_rsi_attestation_token_continue(uint64_t addr);
uint64_t val_realm_rsi_attestation_token_init(uint64_t addr, uint64_t challenge_0,
                 uint64_t challenge_1, uint64_t challenge_2, uint64_t challenge_3,
                 uint64_t challenge_4, uint64_t challenge_5, uint64_t challenge_6,
                                                            uint64_t challenge_7);
uint64_t val_realm_rsi_measurement_extend(uint64_t index, uint64_t size, uint64_t value_0,
                                     uint64_t value_1, uint64_t value_2, uint64_t value_3,
                                     uint64_t value_4, uint64_t value_5, uint64_t value_6,
                                                                        uint64_t value_7);
val_smc_param_ts val_realm_rsi_measurement_read(uint64_t index);
#endif /* _VAL_REALM_RMI_H_ */
