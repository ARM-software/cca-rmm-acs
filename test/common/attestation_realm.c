/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "attestation_realm.h"
#include "mbedtls/md.h"

/* One bit for each mandatory claim in DEN0137 A7.2.3. */
#define REALM_REQUIRED_CLAIMS    0x1ffU
#define PLATFORM_REQUIRED_CLAIMS 0x1ffU

static bool valid_hash_size(size_t size)
{
    return size == CCA_BYTE_SIZE_32 || size == CCA_BYTE_SIZE_48 ||
           size == CCA_BYTE_SIZE_64;
}

static bool bytes_equal(const void *left, const void *right, size_t len)
{
    const uint8_t *a = left;
    const uint8_t *b = right;
    size_t i;

    for (i = 0; i < len; i++)
        if (a[i] != b[i])
            return false;
    return true;
}

/* This ACS verifier currently supports these IANA hash names. */
static size_t hash_size_from_algo(struct q_useful_buf_c algo)
{
    if (algo.len == 7 && bytes_equal(algo.ptr, "sha-256", 7))
        return CCA_BYTE_SIZE_32;
    if (algo.len == 7 && bytes_equal(algo.ptr, "sha-384", 7))
        return CCA_BYTE_SIZE_48;
    if (algo.len == 7 && bytes_equal(algo.ptr, "sha-512", 7))
        return CCA_BYTE_SIZE_64;
    return 0;
}

static uint64_t verify_measurement_sizes(const attestation_token_ts *token,
                                         size_t default_platform_measurement_size)
{
    size_t realm_size = hash_size_from_algo(token->realm_hash_algo);
    size_t platform_size = hash_size_from_algo(token->platform_hash_algo);
    int i;

    if (realm_size == 0 || platform_size == 0 ||
        token->realm_initial_measurement.len != realm_size ||
        (default_platform_measurement_size != 0 &&
         default_platform_measurement_size != platform_size))
        return VAL_ERROR;

    for (i = 0; i < 4; i++)
        if (token->rem[i].len != realm_size)
            return VAL_ERROR;

    return VAL_SUCCESS;
}

static uint64_t verify_workload_binding(const attestation_token_ts *token)
{
    const mbedtls_md_info_t *info;
    mbedtls_md_type_t type;
    struct q_useful_buf_c algo = token->realm_public_key_hash_algo;
    uint8_t digest[CCA_BYTE_SIZE_64];

    if (algo.len == 7 && bytes_equal(algo.ptr, "sha-256", 7))
        type = MBEDTLS_MD_SHA256;
    else if (algo.len == 7 && bytes_equal(algo.ptr, "sha-384", 7))
        type = MBEDTLS_MD_SHA384;
    else if (algo.len == 7 && bytes_equal(algo.ptr, "sha-512", 7))
        type = MBEDTLS_MD_SHA512;
    else
        return VAL_ERROR;

    info = mbedtls_md_info_from_type(type);
    if (info == NULL || token->realm_public_key.ptr == NULL ||
        token->platform_workload_binding.ptr == NULL ||
        token->platform_workload_binding.len != mbedtls_md_get_size(info))
        return VAL_ERROR;

    if (mbedtls_md(info, token->realm_public_key.ptr,
                   token->realm_public_key.len, digest) != 0)
        return VAL_ERROR;

    return bytes_equal(digest, token->platform_workload_binding.ptr,
                       token->platform_workload_binding.len) ? VAL_SUCCESS : VAL_ERROR;
}


/* Validate the optional platform RoTPK claim. */
static uint64_t parse_platform_tbb_rotpk(QCBORDecodeContext *ctx, QCBORItem item)
{
    int entry, field;

    if (item.uDataType != QCBOR_TYPE_ARRAY || item.val.uCount == 0)
        return VAL_ERROR;

    for (entry = 0; entry < item.val.uCount; entry++)
    {
        uint32_t seen = 0;
        int field_count;

        if (QCBORDecode_GetNext(ctx, &item) != QCBOR_SUCCESS ||
            item.uDataType != QCBOR_TYPE_MAP)
            return VAL_ERROR;
        field_count = item.val.uCount;
        for (field = 0; field < field_count; field++)
        {
            unsigned int label;
            uint32_t bit;

            if (QCBORDecode_GetNext(ctx, &item) != QCBOR_SUCCESS ||
                item.uLabelType != QCBOR_TYPE_INT64 ||
                item.label.int64 < 1 || item.label.int64 > 3)
                return VAL_ERROR;
            label = (unsigned int)item.label.int64;
            bit = 1U << label;
            if ((seen & bit) != 0U)
                return VAL_ERROR;
            seen |= bit;

            if ((label == 1U && item.uDataType != QCBOR_TYPE_TEXT_STRING) ||
                (label == 2U && item.uDataType != QCBOR_TYPE_INT64) ||
                (label == 3U && (item.uDataType != QCBOR_TYPE_BYTE_STRING ||
                                 !valid_hash_size(item.val.string.len))))
                return VAL_ERROR;
        }
        if (seen != 0x0eU)
            return VAL_ERROR;
    }
    return VAL_SUCCESS;
}

static bool same_cbor_label(const QCBORItem *left, const QCBORItem *right)
{
    if (left->uLabelType != right->uLabelType)
        return false;

    switch (left->uLabelType)
    {
    case QCBOR_TYPE_INT64:
        return left->label.int64 == right->label.int64;
    case QCBOR_TYPE_UINT64:
        return left->label.uint64 == right->label.uint64;
    case QCBOR_TYPE_TEXT_STRING:
    case QCBOR_TYPE_BYTE_STRING:
        return UsefulBuf_Compare(left->label.string, right->label.string) == 0;
    default:
        return false;
    }
}

/* Validate the whole claim map and reject repeated labels. */
static uint64_t validate_single_cbor_map(UsefulBufC encoded)
{
    QCBORDecodeContext ctx, prior_ctx;
    QCBORItem map, current, prior;
    size_t index = 0, previous;

    QCBORDecode_Init(&ctx, encoded, QCBOR_DECODE_MODE_NORMAL);
    if (QCBORDecode_GetNext(&ctx, &map) != QCBOR_SUCCESS ||
        map.uDataType != QCBOR_TYPE_MAP)
        return VAL_ERROR;

    if (map.uNextNestLevel > map.uNestingLevel)
    {
        do
        {
            QCBORDecode_VGetNextConsume(&ctx, &current);
            if (QCBORDecode_GetError(&ctx) != QCBOR_SUCCESS ||
                current.uLabelType == QCBOR_TYPE_NONE)
                return VAL_ERROR;

            QCBORDecode_Init(&prior_ctx, encoded, QCBOR_DECODE_MODE_NORMAL);
            if (QCBORDecode_GetNext(&prior_ctx, &prior) != QCBOR_SUCCESS)
                return VAL_ERROR;
            for (previous = 0; previous < index; previous++)
            {
                QCBORDecode_VGetNextConsume(&prior_ctx, &prior);
                if (QCBORDecode_GetError(&prior_ctx) != QCBOR_SUCCESS)
                    return VAL_ERROR;
                if (same_cbor_label(&current, &prior))
                    return VAL_ERROR;
            }
            index++;
        } while (current.uNextNestLevel > map.uNestingLevel);
    }

    return QCBORDecode_Finish(&ctx) == QCBOR_SUCCESS ? VAL_SUCCESS : VAL_ERROR;
}

/**
    @brief    - API to validate Realm Public Key structure,
    @param    - KeyBstr : Realm Public Key binary string.
    @return   - Error status.
**/
static uint64_t validate_realm_pub_key(UsefulBufC KeyBstr)
{
    QCBORDecodeContext ctx;
    QCBORItem item;
    uint32_t seen_common = 0U;
    uint32_t i, j, count;

    QCBORDecode_Init(&ctx, KeyBstr, QCBOR_DECODE_MODE_NORMAL);
    if (QCBORDecode_GetNext(&ctx, &item) != QCBOR_SUCCESS ||
        item.uDataType != QCBOR_TYPE_MAP)
        return VAL_ERROR;

    count = item.val.uCount;
    for (i = 0; i < count; i++)
    {
        uint32_t bit = 0U;
        uint32_t ops_count;
        uint8_t nesting_level;

        if (QCBORDecode_GetNext(&ctx, &item) != QCBOR_SUCCESS)
            return VAL_ERROR;

        if (item.uLabelType == QCBOR_TYPE_INT64)
        {
            switch (item.label.int64)
            {
            case CCA_REALM_PUBLIC_KEY_TYPE:
                bit = 1U << 0;
                if (item.uDataType != QCBOR_TYPE_INT64 &&
                    item.uDataType != QCBOR_TYPE_TEXT_STRING)
                    return VAL_ERROR;
                if ((item.uDataType == QCBOR_TYPE_INT64 && item.val.int64 == 0) ||
                    (item.uDataType == QCBOR_TYPE_TEXT_STRING &&
                     item.val.string.len == 0U))
                    return VAL_ERROR;
                break;
            case CCA_REALM_PUBLIC_KEY_ID:
                bit = 1U << 1;
                if (item.uDataType != QCBOR_TYPE_BYTE_STRING)
                    return VAL_ERROR;
                break;
            case CCA_REALM_PUBLIC_KEY_ALGO:
                bit = 1U << 2;
                if (item.uDataType != QCBOR_TYPE_INT64 &&
                    item.uDataType != QCBOR_TYPE_TEXT_STRING)
                    return VAL_ERROR;
                break;
            case CCA_REALM_PUBLIC_KEY_OPS:
                bit = 1U << 3;
                if (item.uDataType != QCBOR_TYPE_ARRAY || item.val.uCount == 0U)
                    return VAL_ERROR;
                ops_count = item.val.uCount;
                for (j = 0; j < ops_count; j++)
                {
                    if (QCBORDecode_GetNext(&ctx, &item) != QCBOR_SUCCESS ||
                        (item.uDataType != QCBOR_TYPE_INT64 &&
                         item.uDataType != QCBOR_TYPE_TEXT_STRING))
                        return VAL_ERROR;
                }
                break;
            case CCA_REALM_PUBLIC_KEY_BASE_IV:
                bit = 1U << 4;
                if (item.uDataType != QCBOR_TYPE_BYTE_STRING)
                    return VAL_ERROR;
                break;
            default:
                break;
            }
        }
        if (bit != 0U)
        {
            if ((seen_common & bit) != 0U)
                return VAL_ERROR;
            seen_common |= bit;
        }
        else
        {
            /* Key-type-specific fields are not checked here. */
            nesting_level = item.uNestingLevel;
            while (item.uNextNestLevel > nesting_level)
            {
                if (QCBORDecode_GetNext(&ctx, &item) != QCBOR_SUCCESS)
                    return VAL_ERROR;
            }
        }
    }

    if ((seen_common & 1U) == 0U || QCBORDecode_Finish(&ctx) != QCBOR_SUCCESS)
        return VAL_ERROR;

    return VAL_SUCCESS;
}

/**
    @brief    - This API will verify the claims for realm token
    @param    - decode_context      : The buffer containing the challenge
                item                : context for decoding the data items
                completed_challenge : Buffer containing the challenge
    @return   - error status
**/
static uint64_t parse_claims_realm_token(attestation_token_ts *attestation_token,
                                         QCBORDecodeContext *decode_context, QCBORItem item,
                                         struct q_useful_buf_c completed_challenge)
{
    uint64_t status = VAL_SUCCESS;
    int i, count = 0;

    /* Parse each claim and validate their data type */
    while (status == VAL_SUCCESS)
    {
        status = QCBORDecode_GetNext(decode_context, &item);
        if (status != VAL_SUCCESS)
            break;

        if (item.uLabelType == QCBOR_TYPE_INT64)
        {
            if (item.label.int64 == CCA_REALM_CHALLENGE)
            {
                attestation_token->realm_claims_seen |= 1U << 0;
                if (item.uDataType == QCBOR_TYPE_BYTE_STRING &&
                    item.val.string.len == CCA_BYTE_SIZE_64)
                {
                    attestation_token->challenge = item.val.string;
                    /* Given challenge vs challenge in token */
                    if (UsefulBuf_Compare(item.val.string, completed_challenge))
                    {
                        LOG(ERROR, "Realm challenge and given challenge are not same.\n");
                        return VAL_ERROR;
                    }
                }
                else
                {
                    LOG(ERROR, "Realm challenge is not in expected format.\n");
                    return VAL_ERROR;
                }
            }
            else if (item.label.int64 == CCA_REALM_INSTANCE_ID)
            {
                if (item.uDataType != QCBOR_TYPE_BYTE_STRING ||
                    item.val.string.len != CCA_BYTE_SIZE_33 ||
                    ((const uint8_t *)item.val.string.ptr)[0] != 1U)
                    return VAL_ERROR;
                attestation_token->realm_claims_seen |= 1U << 1;
            }
            else if (item.label.int64 == CCA_REALM_PROFILE)
            {
                static const char profile[] = "tag:arm.com,2026:realm#2.0.0";

                if (item.uDataType != QCBOR_TYPE_TEXT_STRING ||
                    item.val.string.len != sizeof(profile) - 1U ||
                    !bytes_equal(item.val.string.ptr, profile, sizeof(profile) - 1U))
                    return VAL_ERROR;
            }
            else if (item.label.int64 == CCA_REALM_LFA_POLICY)
            {
                if (item.uDataType != QCBOR_TYPE_INT64 ||
                    (item.val.int64 != 0 && item.val.int64 != 1))
                    return VAL_ERROR;
            }
            else if (item.label.int64 == CCA_REALM_PERSONALIZATION_VALUE)
            {
                if (item.uDataType != QCBOR_TYPE_BYTE_STRING)
                {
                    LOG(ERROR, "Realm personalization value is not in expected format.\n");
                    return VAL_ERROR;
                }
                if (item.val.string.len != CCA_BYTE_SIZE_64)
                {
                    LOG(ERROR, "Realm personalization value size is incorrect.\n");
                    return VAL_ERROR;
                }
                attestation_token->rpv = item.val.string;
                attestation_token->realm_claims_seen |= 1U << 2;
            }
            else if (item.label.int64 == CCA_REALM_INITIAL_MEASUREMENT)
            {
                if (item.uDataType != QCBOR_TYPE_BYTE_STRING)
                {
                    LOG(ERROR, "Realm initial measurement is not in expected format.\n");
                    return VAL_ERROR;
                }
                if (item.val.string.len != CCA_BYTE_SIZE_32 &&
                    item.val.string.len != CCA_BYTE_SIZE_48 &&
                    item.val.string.len != CCA_BYTE_SIZE_64)
                {
                    LOG(ERROR, "Realm initial measurement is not in given size format.\n");
                    return VAL_ERROR;
                }
                attestation_token->realm_initial_measurement = item.val.string;
                attestation_token->realm_claims_seen |= 1U << 3;
            }
            else if (item.label.int64 == CCA_REALM_PUBLIC_KEY)
            {
                if (item.uDataType != QCBOR_TYPE_BYTE_STRING)
                {
                    LOG(ERROR, "Realm public key is not in expected format.\n");
                    return VAL_ERROR;
                }

                /* Realm public key should be of type COSE_Key defined in RFC 8152 protocol*/
                if (validate_realm_pub_key(item.val.string)) {
                    LOG(ERROR, "Realm public key is not in expected format.\n");
                    return VAL_ERROR;
                }
                attestation_token->realm_public_key = item.val.string;
                attestation_token->realm_claims_seen |= 1U << 6;
            }
            else if (item.label.int64 == CCA_REALM_HASH_ALGO_ID ||
                     item.label.int64 == CCA_REALM_PUBLIC_KEY_HASH_ALGO_ID)
            {
                if (item.uDataType != QCBOR_TYPE_TEXT_STRING)
                {
                    LOG(ERROR, "Realm hash algo/public key hash algo id is \
                                                 not in expected format.\n");
                    return VAL_ERROR;
                }
                if (item.label.int64 == CCA_REALM_HASH_ALGO_ID)
                {
                    attestation_token->realm_hash_algo = item.val.string;
                    attestation_token->realm_claims_seen |= 1U << 5;
                }
                else
                {
                    attestation_token->realm_public_key_hash_algo = item.val.string;
                    attestation_token->realm_claims_seen |= 1U << 7;
                }
            }
            else if (item.label.int64 == CCA_REALM_MEC_POLICY)
            {
                if (item.uDataType != QCBOR_TYPE_INT64 ||
                    (item.val.int64 != 0 && item.val.int64 != 1))
                    return VAL_ERROR;
                attestation_token->realm_claims_seen |= 1U << 8;
            }
            else if (item.label.int64 == CCA_REALM_EXTENSIBLE_MEASUREMENT)
            {
                if (item.uDataType != QCBOR_TYPE_ARRAY)
                {
                    LOG(ERROR, "Realm extensible measurement is not in expected format.\n");
                    return VAL_ERROR;
                }

                count = item.val.uCount;
                if (count != 4)
                    return VAL_ERROR;
                for (i = 0; i < count; i++)
                {
                    status = QCBORDecode_GetNext(decode_context, &item);
                    if (status != QCBOR_SUCCESS ||
                        item.uDataType != QCBOR_TYPE_BYTE_STRING ||
                        !valid_hash_size(item.val.string.len))
                        return VAL_ERROR;
                    attestation_token->rem[i] = item.val.string;
                }
                attestation_token->realm_claims_seen |= 1U << 4;
            }
        }
        else
        {
            /* For other claim types */
        }
    }

    if (status == QCBOR_ERR_NO_MORE_ITEMS)
        return VAL_SUCCESS;
    else
        return VAL_ERROR;
}

/**
    @brief    - This API will verify the claims for platform token
    @param    - decode_context      : The buffer containing the challenge
                item                : context for decoding the data items
    @return   - error status
**/
static uint64_t parse_claims_platform_token(attestation_token_ts *attestation_token,
                                            QCBORDecodeContext *decode_context, QCBORItem item,
                                            size_t *default_measurement_size)
{
    int i, count = 0, sw_comp_count = 0, index = 0;
    uint64_t status = VAL_SUCCESS;

    /* Parse each claim and validate their data type */
    while (status == VAL_SUCCESS)
    {
        status = QCBORDecode_GetNext(decode_context, &item);
        if (status != VAL_SUCCESS)
            break;

        if (item.uLabelType == QCBOR_TYPE_INT64)
        {
            if (item.label.int64 == CCA_PLATFORM_PROFILE ||
                item.label.int64 == CCA_PLATFORM_VERIFICATION_SERVICE ||
                item.label.int64 == CCA_PLATFORM_HASH_ALGO_ID)
            {

                if (item.uDataType != QCBOR_TYPE_TEXT_STRING)
                {
                    LOG(ERROR, "Platform token profile/verification-service/hash algo is \
                                                              not in expected format.\n");
                    return VAL_ERROR;
                }
                if (item.label.int64 == CCA_PLATFORM_PROFILE)
                {
                    static const char profile[] = "tag:arm.com,2026:cca_platform#2.0.0";
                    static const char delegated[] =
                        "tag:arm.com,2026:cca_platform#2.0.0;delegated";
                    if (!((item.val.string.len == sizeof(profile) - 1U &&
                           bytes_equal(item.val.string.ptr, profile,
                                       sizeof(profile) - 1U)) ||
                          (item.val.string.len == sizeof(delegated) - 1U &&
                           bytes_equal(item.val.string.ptr, delegated,
                                       sizeof(delegated) - 1U))))
                        return VAL_ERROR;
                    attestation_token->platform_profile = item.val.string;
                    attestation_token->platform_claims_seen |= 1U << 0;
                }
                else if (item.label.int64 == CCA_PLATFORM_HASH_ALGO_ID)
                {
                    attestation_token->platform_hash_algo = item.val.string;
                    attestation_token->platform_claims_seen |= 1U << 7;
                }
            }
            else if (item.label.int64 == CCA_PLATFORM_CHALLENGE)
            {

                if (item.uDataType != QCBOR_TYPE_BYTE_STRING)
                {
                    LOG(ERROR, "Platform challenge is not in expected format.\n");
                    return VAL_ERROR;
                }
                if (item.val.string.len != CCA_BYTE_SIZE_32 &&
                    item.val.string.len != CCA_BYTE_SIZE_48 &&
                    item.val.string.len != CCA_BYTE_SIZE_64)
                {
                    LOG(ERROR, "Platform challenge size is incorrect.\n");
                    return VAL_ERROR;
                }
                attestation_token->platform_attest_challenge = item.val.string;
            }
            else if (item.label.int64 == CCA_PLATFORM_TBB_ROTPK)
            {
                if (parse_platform_tbb_rotpk(decode_context, item) != VAL_SUCCESS)
                    return VAL_ERROR;
            }
            else if (item.label.int64 == CCA_PLATFORM_MANUFACTURING_CONFIG ||
                     item.label.int64 == CCA_PLATFORM_DISCRETE_TPM_BINDING)
            {
                if (item.uDataType != QCBOR_TYPE_BYTE_STRING)
                    return VAL_ERROR;
            }
            else if (item.label.int64 == CCA_PLATFORM_WORKLOAD_BINDING)
            {
                if (item.uDataType != QCBOR_TYPE_BYTE_STRING ||
                    !valid_hash_size(item.val.string.len))
                    return VAL_ERROR;
                attestation_token->platform_workload_binding = item.val.string;
                attestation_token->platform_claims_seen |= 1U << 1;
            }
            else if (item.label.int64 == CCA_PLATFORM_CLIENT_ID)
            {
                if (item.uDataType != QCBOR_TYPE_INT64 || item.val.int64 != 1)
                    return VAL_ERROR;
                attestation_token->platform_claims_seen |= 1U << 8;
            }
            else if (item.label.int64 == CCA_PLATFORM_IMPLEMENTATION_ID)
            {
                if (item.uDataType != QCBOR_TYPE_BYTE_STRING)
                {
                    LOG(ERROR, "Platform implementation id is not in expected format.\n");
                    return VAL_ERROR;
                }
                if (item.val.string.len != CCA_BYTE_SIZE_32)
                {
                    LOG(ERROR, "Platform implementation id size is incorrect.\n");
                    return VAL_ERROR;
                }
                attestation_token->platform_claims_seen |= 1U << 2;
            }
            else if (item.label.int64 == CCA_PLATFORM_INSTANCE_ID)
            {
                if (item.uDataType != QCBOR_TYPE_BYTE_STRING)
                {
                    LOG(ERROR, "Platform instance id is not in expected format.\n");
                    return VAL_ERROR;
                }
                if (item.val.string.len != CCA_BYTE_SIZE_33)
                {
                    LOG(ERROR, "Platform instance id size is incorrect.\n");
                    return VAL_ERROR;
                }
                if (((const uint8_t *)item.val.string.ptr)[0] != 1U)
                    return VAL_ERROR;
                attestation_token->platform_claims_seen |= 1U << 3;
            }
            else if (item.label.int64 == CCA_PLATFORM_CONFIG)
            {
                if (item.uDataType != QCBOR_TYPE_BYTE_STRING)
                {
                    LOG(ERROR, "Platform config is not in expected format.\n");
                    return VAL_ERROR;
                }
                attestation_token->platform_claims_seen |= 1U << 4;
            }
            else if (item.label.int64 == CCA_PLATFORM_LIFECYCLE)
            {
                /* DEN0137: 0x0000..0x00ff, 0x1000..0x10ff, ...,
                 * 0x6000..0x60ff. The low byte is implementation defined. */
                if (item.uDataType != QCBOR_TYPE_INT64 ||
                    item.val.int64 < 0 || item.val.int64 > 0x60ff ||
                    (item.val.int64 & 0x0f00) != 0)
                {
                    LOG(ERROR, "Platform lifecycle is not in expected range.\n");
                    return VAL_ERROR;
                }
                attestation_token->platform_claims_seen |= 1U << 5;
            }
            else if (item.label.int64 == CCA_PLATFORM_SW_COMPONENTS)
            {
                if (item.uDataType != QCBOR_TYPE_ARRAY)
                {
                    LOG(ERROR, "Software components is not in expected format.\n");
                    return VAL_ERROR;
                }
                sw_comp_count = item.val.uCount;
                if (sw_comp_count == 0)
                    return VAL_ERROR;
                for (index = 0; index < sw_comp_count; index++)
                {
                    uint32_t sw_required = 0;
                    uint32_t sw_fields = 0;
                    size_t measurement_size = 0;
                    struct q_useful_buf_c component_algo = {NULL, 0};
                    status = QCBORDecode_GetNext(decode_context, &item);
                    if (status != QCBOR_SUCCESS)
                        return VAL_ERROR;

                    if (item.uDataType != QCBOR_TYPE_MAP)
                        return VAL_ERROR;
                    count = item.val.uCount;
                    for (i = 0; i < count; i++)
                    {
                        status = QCBORDecode_GetNext(decode_context, &item);
                        if (status != QCBOR_SUCCESS ||
                            item.uLabelType != QCBOR_TYPE_INT64 ||
                            item.label.int64 < 1 || item.label.int64 > 8 ||
                            item.label.int64 == 3)
                            return VAL_ERROR;
                        if ((sw_fields & (1U << (item.label.int64 - 1))) != 0U)
                            return VAL_ERROR;
                        sw_fields |= 1U << (item.label.int64 - 1);

                        if (item.label.int64 == CCA_PLATFORM_SW_COMPONENT_TYPE ||
                            item.label.int64 == CCA_PLATFORM_SW_COMPONENT_VERSION ||
                            item.label.int64 == CCA_PLATFORM_SW_COMPONENT_ALGORITHM_ID)
                        {
                            if (item.uDataType != QCBOR_TYPE_TEXT_STRING)
                                return VAL_ERROR;
                            if (item.label.int64 == CCA_PLATFORM_SW_COMPONENT_ALGORITHM_ID)
                                component_algo = item.val.string;
                        }
                        else if (item.label.int64 == CCA_PLATFORM_SW_COMPONENT_MEASUREMENT_VALUE ||
                                 item.label.int64 == CCA_PLATFORM_SW_COMPONENT_SIGNER_ID)
                        {
                            if (item.uDataType != QCBOR_TYPE_BYTE_STRING ||
                                !valid_hash_size(item.val.string.len))
                                return VAL_ERROR;
                            if (item.label.int64 == CCA_PLATFORM_SW_COMPONENT_MEASUREMENT_VALUE)
                            {
                                measurement_size = item.val.string.len;
                                sw_required |= 1U;
                            }
                            else
                                sw_required |= 2U;
                        }
                        else if (item.label.int64 == CCA_PLATFORM_SW_COMPONENT_LFA_SUPPORTED)
                        {
                            if (item.uDataType != QCBOR_TYPE_TRUE &&
                                item.uDataType != QCBOR_TYPE_FALSE)
                                return VAL_ERROR;
                        }
                        else if (item.label.int64 == CCA_PLATFORM_SW_COMPONENT_COUNTERSIGNERS)
                        {
                            int j, digest_count;

                            if (item.uDataType != QCBOR_TYPE_ARRAY || item.val.uCount == 0)
                                return VAL_ERROR;
                            digest_count = item.val.uCount;
                            for (j = 0; j < digest_count; j++)
                            {
                                status = QCBORDecode_GetNext(decode_context, &item);
                                if (status != QCBOR_SUCCESS ||
                                    item.uDataType != QCBOR_TYPE_BYTE_STRING ||
                                    !valid_hash_size(item.val.string.len))
                                    return VAL_ERROR;
                            }
                        }
                    }

                    if (sw_required != 3U) {
                        LOG(ERROR, " mandatory sw_components fields are absent\n");
                        return VAL_ERROR;
                    }

                    if (component_algo.ptr != NULL)
                    {
                        if (hash_size_from_algo(component_algo) != measurement_size)
                            return VAL_ERROR;
                    }
                    else if (*default_measurement_size != 0 &&
                             *default_measurement_size != measurement_size)
                        return VAL_ERROR;
                    else
                        *default_measurement_size = measurement_size;
                } /*for (index = 0; index<sw_comp_count; index++)*/
                attestation_token->platform_claims_seen |= 1U << 6;
            }
        }
        else
        {
            /* For other claim types */
        }
    }

    if (status == QCBOR_ERR_NO_MORE_ITEMS)
        return VAL_SUCCESS;
    else
    {
        LOG(ERROR, "Realm challenge is not in expected format.\n");
        return VAL_ERROR;
    }
}

uint64_t val_attestation_verify_token(attestation_token_ts *attestation_token,
            uint64_t *challenge, __attribute__((unused)) size_t challenge_size,
                                        uint64_t *token, size_t token_size)
{
    uint64_t               status = VAL_SUCCESS;
    QCBORItem             item;
    QCBORDecodeContext    decode_context;
    struct q_useful_buf_c completed_challenge;
    struct q_useful_buf_c completed_token;
    struct q_useful_buf_c platform_token_payload;
    struct q_useful_buf_c realm_token_payload;
    struct q_useful_buf_c payload1;
    struct q_useful_buf_c payload2;
    size_t default_platform_measurement_size = 0;
    uint32_t cmw_seen = 0U;
    unsigned int entry;

    val_memset(attestation_token, 0, sizeof(*attestation_token));

    /* Construct the token buffer for validation */
    completed_token.ptr = token;
    completed_token.len = token_size;

    /* Construct the challenge buffer for validation */
    completed_challenge.ptr = challenge;
    completed_challenge.len = challenge_size;

/*      COSE_Sign1 Format
    -------------------------
    |  CBOR Array Type      |
    -------------------------
    |  Protected Headers    |
    -------------------------
    |  Unprotected Headers  |
    -------------------------
    |  Payload              |
    -------------------------
    |  Signature            |
    -------------------------
*/

    /* Initialize the decorder */
    QCBORDecode_Init(&decode_context, completed_token, QCBOR_DECODE_MODE_NORMAL);

    /* Get the Header */
    status = QCBORDecode_GetNext(&decode_context, &item);
    if (status != QCBOR_SUCCESS)
        return VAL_ERROR;

    /* Check the CBOR Map type. Check if the count is 2.
     * Only COSE_SIGN1 is supported now.
     */
    if (item.uDataType != QCBOR_TYPE_MAP || item.val.uCount != 2 ||
        !QCBORDecode_IsTagged(&decode_context, &item, CCA_TOKEN_CMW_TAG))
    {
        LOG(ERROR, " Attestation token error formatting\n");
        return VAL_ERROR;
    }

    /* CMW is a map, so its platform and Realm entries may appear in either order. */
    for (entry = 0; entry < 2U; entry++)
    {
        int64_t entry_label;

        status = QCBORDecode_GetNext(&decode_context, &item);
        if (status != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_ARRAY ||
            item.val.uCount != 2 || item.uLabelType != QCBOR_TYPE_INT64)
            return VAL_ERROR;
        entry_label = item.label.int64;

        status = QCBORDecode_GetNext(&decode_context, &item);
        if (status != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_INT64 ||
            item.val.int64 != CCA_TOKEN_CWT_TYPE)
            return VAL_ERROR;

        status = QCBORDecode_GetNext(&decode_context, &item);
        if (status != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_BYTE_STRING)
            return VAL_ERROR;

        if (entry_label == CCA_PLATFORM_TOKEN && (cmw_seen & 1U) == 0U)
        {
            platform_token_payload = item.val.string;
            cmw_seen |= 1U;
        }
        else if (entry_label == CCA_REALM_TOKEN && (cmw_seen & 2U) == 0U)
        {
            realm_token_payload = item.val.string;
            cmw_seen |= 2U;
        }
        else
            return VAL_ERROR;
    }
    if (cmw_seen != 3U || QCBORDecode_Finish(&decode_context) != QCBOR_SUCCESS)
        return VAL_ERROR;

    /* Initialize the Decoder and validate the cca-platform token payload format */
    QCBORDecode_Init(&decode_context, platform_token_payload, QCBOR_DECODE_MODE_NORMAL);
    status = QCBORDecode_GetNext(&decode_context, &item);
    if (status != VAL_SUCCESS)
        return status;

    if (item.uDataType != QCBOR_TYPE_ARRAY || item.val.uCount != 4 ||
            !QCBORDecode_IsTagged(&decode_context, &item, CBOR_TAG_COSE_SIGN1))
    {
        LOG(ERROR, " Attestation token error formatting\n");
        return VAL_ERROR;
    }
    status = QCBORDecode_GetNext(&decode_context, &item);
    if (status != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_BYTE_STRING)
    {
        LOG(ERROR, "COSE protected headers must be a byte string.\n");
        return VAL_ERROR;
    }

    QCBORDecode_VGetNextConsume(&decode_context, &item);
    if (QCBORDecode_GetError(&decode_context) != QCBOR_SUCCESS ||
        item.uDataType != QCBOR_TYPE_MAP)
    {
        LOG(ERROR, "COSE unprotected headers must be a map.\n");
        return VAL_ERROR;
    }

    status = QCBORDecode_GetNext(&decode_context, &item);
    if (status != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_BYTE_STRING)
    {
        LOG(ERROR, "COSE payload must be a byte string.\n");
        return VAL_ERROR;
    }

    payload1 = item.val.string;
    status = QCBORDecode_GetNext(&decode_context, &item);
    if (status != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_BYTE_STRING)
    {
        LOG(ERROR, "Platform token signature is not a byte string.\n");
        return VAL_ERROR;
    }

    if (QCBORDecode_Finish(&decode_context) != QCBOR_SUCCESS)
        return VAL_ERROR;

    /* Verify the signature */
    status = pal_verify_signature(token);
    if (status != VAL_SUCCESS)
        return status;

    if (validate_single_cbor_map(payload1) != VAL_SUCCESS)
        return VAL_ERROR;
    QCBORDecode_Init(&decode_context, payload1, QCBOR_DECODE_MODE_NORMAL);
    status = QCBORDecode_GetNext(&decode_context, &item);
    if (status != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_MAP)
    {
        LOG(ERROR, " Attestation token error formatting\n");
        return VAL_ERROR;
    }
    /* Parse the payload and check the data type of each claim */
    status = parse_claims_platform_token(attestation_token, &decode_context, item,
                                         &default_platform_measurement_size);
    if (status != VAL_SUCCESS || QCBORDecode_Finish(&decode_context) != QCBOR_SUCCESS)
        return VAL_ERROR;

    /* Initialize the Decoder and validate the realm token payload format */
    QCBORDecode_Init(&decode_context, realm_token_payload, QCBOR_DECODE_MODE_NORMAL);
    status = QCBORDecode_GetNext(&decode_context, &item);
    if (status != VAL_SUCCESS)
        return status;

    if (item.uDataType != QCBOR_TYPE_ARRAY || item.val.uCount != 4 ||
            !QCBORDecode_IsTagged(&decode_context, &item, CBOR_TAG_COSE_SIGN1))
    {
        LOG(ERROR, " Attestation token error formatting\n");
        return VAL_ERROR;
    }

    status = QCBORDecode_GetNext(&decode_context, &item);
    if (status != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_BYTE_STRING)
    {
        LOG(ERROR, "COSE protected headers must be a byte string.\n");
        return VAL_ERROR;
    }

    QCBORDecode_VGetNextConsume(&decode_context, &item);
    if (QCBORDecode_GetError(&decode_context) != QCBOR_SUCCESS ||
        item.uDataType != QCBOR_TYPE_MAP)
    {
        LOG(ERROR, "COSE unprotected headers must be a map.\n");
        return VAL_ERROR;
    }

    status = QCBORDecode_GetNext(&decode_context, &item);
    if (status != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_BYTE_STRING)
    {
        LOG(ERROR, "COSE payload must be a byte string.\n");
        return VAL_ERROR;
    }

    payload2 = item.val.string;
    status = QCBORDecode_GetNext(&decode_context, &item);
    if (status != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_BYTE_STRING)
    {
        LOG(ERROR, "Realm token signature is not a byte string.\n");
        return VAL_ERROR;
    }
    if (QCBORDecode_Finish(&decode_context) != QCBOR_SUCCESS)
        return VAL_ERROR;
    if (validate_single_cbor_map(payload2) != VAL_SUCCESS)
        return VAL_ERROR;
    QCBORDecode_Init(&decode_context, payload2, QCBOR_DECODE_MODE_NORMAL);


    status = QCBORDecode_GetNext(&decode_context, &item);
    if (status != QCBOR_SUCCESS || item.uDataType != QCBOR_TYPE_MAP)
    {
        LOG(ERROR, " Attestation token error formatting\n");
        return VAL_ERROR;
    }

    /* Parse the payload and check the data type of each claim */
    status = parse_claims_realm_token(attestation_token, &decode_context, item,
                                                          completed_challenge);
    if (status != VAL_SUCCESS || QCBORDecode_Finish(&decode_context) != QCBOR_SUCCESS)
        return VAL_ERROR;

    if (attestation_token->realm_claims_seen != REALM_REQUIRED_CLAIMS)
    {
        LOG(ERROR, " mandatory realm claims are absent.\n");
        return VAL_ERROR;
    }

    if (attestation_token->platform_claims_seen != PLATFORM_REQUIRED_CLAIMS) {
        LOG(ERROR, " mandatory platform claims are absent.\n");
        return VAL_ERROR;
    }

    if (attestation_token->platform_attest_challenge.ptr != NULL) {
        LOG(ERROR, "Platform challenge is unexpected for delegated profile.\n");
        return VAL_ERROR;
    }

    if (verify_measurement_sizes(attestation_token,
                                 default_platform_measurement_size) != VAL_SUCCESS) {
        LOG(ERROR, "Hash algorithm or measurement size is inconsistent.\n");
        return VAL_ERROR;
    }

    if (verify_workload_binding(attestation_token) != VAL_SUCCESS) {
        LOG(ERROR, "Platform workload binding does not match Realm public key.\n");
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}
