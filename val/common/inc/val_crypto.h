/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _VAL_CRYPTO_H_
#define _VAL_CRYPTO_H_

#include "pal_crypto.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/oid.h"

#define PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P256    0x10
#define PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P384    0x20
#define PUBLIC_KEY_ALGO_RSASSA_3072        0x30

int val_host_get_public_key_from_cert_chain(uint8_t *cert_chain,
                    size_t cert_chain_len,
                    void *public_key,
                    size_t *public_key_len,
                    void *public_key_metadata,
                    size_t *public_key_metadata_len,
                    uint8_t *public_key_algo);

#endif /* _VAL_CRYPTO_H_ */