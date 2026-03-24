/*
 * Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "pal_crypto.h"

/**
  @brief  Call the mbedtls certificate parse function
  @param  chain   - certificate chain struct pointer
  @param  buf     - certificate buffer
  @return Returns zero for success, else mbedtls error code
**/
int pal_mbedtls_x509_crt_parse(mbedtls_x509_crt *chain, const unsigned char *buf, size_t cert_len)
{
	return mbedtls_x509_crt_parse(chain, buf, cert_len);
}

/**
  @brief  Call the mbedtls to get signature algorithm from oid
  @param  oid      OID to use
  @param  desc     place to store string pointer
  @return Returns zero for success, else mbedtls error code
**/
int pal_mbedtls_oid_get_sig_alg_desc(const mbedtls_asn1_buf *oid, const char **desc)
{
    return mbedtls_oid_get_sig_alg_desc(oid, desc);
}