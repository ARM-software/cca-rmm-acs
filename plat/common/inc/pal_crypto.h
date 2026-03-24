/*
 * Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _PAL_CRYPTO_H_
#define _PAL_CRYPTO_H_

#include "mbedtls/x509_crt.h"
#include "mbedtls/oid.h"

int pal_mbedtls_x509_crt_parse(mbedtls_x509_crt * chain, const unsigned char *buf, size_t cert_len);
int pal_mbedtls_oid_get_sig_alg_desc(const mbedtls_asn1_buf *oid, const char **desc);

#endif /* _PAL_CRYPTO_H_ */