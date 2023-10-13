/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_framework.h"
#include "qcbor.h"

#define ATTEST_CHALLENGE_SIZE_32  (32u)
#define ATTEST_CHALLENGE_SIZE_48  (48u)
#define ATTEST_CHALLENGE_SIZE_64  (64u)

#define ATTEST_MAX_TOKEN_SIZE 4096

#define CCA_PLATFORM_TOKEN 44234
#define CCA_REALM_TOKEN 44241

#define CCA_PLATFORM_PROFILE 265
#define CCA_PLATFORM_CHALLENGE 10
#define CCA_PLATFORM_IMPLEMENTATION_ID 239
#define CCA_PLATFORM_INSTANCE_ID 256
#define CCA_PLATFORM_CONFIG 2401
#define CCA_PLATFORM_LIFESTYLE 2395
#define CCA_PLATFORM_SW_COMPONENTS 2399
#define CCA_PLATFORM_VERIFICATION_SERVICE 2400
#define CCA_PLATFORM_HASH_ALGO_ID 2402

#define CCA_PLATFORM_SW_COMPONENT_TYPE 1
#define CCA_PLATFORM_SW_COMPONENT_MEASUREMENT_VALUE 2
#define CCA_PLATFORM_SW_COMPONENT_VERSION 4
#define CCA_PLATFORM_SW_COMPONENT_SIGNER_ID 5
#define CCA_PLATFORM_SW_COMPONENT_ALGORITHM_ID 6

#define CCA_REALM_CHALLENGE 10
#define CCA_REALM_PERSONALIZATION_VALUE 44235
#define CCA_REALM_INITIAL_MEASUREMENT 44238
#define CCA_REALM_EXTENSIBLE_MEASUREMENT 44239
#define CCA_REALM_HASH_ALGO_ID 44236
#define CCA_REALM_PUBLIC_KEY 44237
#define CCA_REALM_PUBLIC_KEY_HASH_ALGO_ID 44240

#define CCA_BYTE_SIZE_32    32
#define CCA_BYTE_SIZE_48    48
#define CCA_BYTE_SIZE_64    64

#define CCA_BYTE_SIZE_33    33
#define CCA_BYTE_SIZE_97    97

typedef struct {
    struct q_useful_buf_c challenge;
    struct q_useful_buf_c rpv;
    struct q_useful_buf_c realm_initial_measurement;
    struct q_useful_buf_c platform_attest_challenge;
    struct q_useful_buf_c rem[4];
} attestation_token_ts;

uint64_t val_attestation_verify_token(attestation_token_ts *attestation_token,
                                   uint64_t *challenge, size_t challenge_size,
                                           uint8_t *token, size_t token_size);