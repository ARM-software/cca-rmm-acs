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

typedef struct _val_realm_rsi_host_call {
        /* imm values passed during the host call */
        uint16_t imm;
        uint8_t unused[6];
        /* gprs values to be saved during the rsi host call */
        uint64_t gprs[7];
        uint8_t unused1[4032];
} val_realm_rsi_host_call_t;

typedef struct {
    uint64_t ipa_width;
} val_realm_rsi_realm_config_ts;

uint64_t val_realm_rsi_version(void);
uint64_t val_realm_rsi_realm_config(uint64_t buff);
uint64_t val_realm_rsi_host_call(uint16_t imm);
val_realm_rsi_host_call_t *val_realm_rsi_host_call_ripas(uint16_t imm);
uint64_t val_realm_get_ipa_width(void);
val_smc_param_ts val_realm_rsi_ipa_state_set(uint64_t base, uint64_t size, uint8_t ripas);
val_smc_param_ts val_realm_rsi_ipa_state_get(uint64_t ipa_base);
#endif /* _VAL_REALM_RMI_H_ */
