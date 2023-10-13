/*
 * Copyright (c) 2023, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "attestation_realm.h"

uint32_t    mandatory_realm_claims = 0;
uint32_t    mandatory_platform_claims = 0;
uint32_t    mandaroty_sw_components = 0;

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
                mandatory_realm_claims = mandatory_realm_claims + 1;
                if (item.uDataType == QCBOR_TYPE_BYTE_STRING &&
                    item.val.string.len == CCA_BYTE_SIZE_64)
                {
                    attestation_token->challenge = item.val.string;
                    /* Given challenge vs challenge in token */
                    if (UsefulBuf_Compare(item.val.string, completed_challenge))
                    {
                        LOG(ERROR, "\tRealm challenge and given challenge are not same.", 0, 0);
                        return VAL_ERROR;
                    }
                }
                else
                {
                    LOG(ERROR, "\tRealm challenge is not in expected format.", 0, 0);
                    return VAL_ERROR;
                }
            }
            else if (item.label.int64 == CCA_REALM_PERSONALIZATION_VALUE)
            {
                mandatory_realm_claims = mandatory_realm_claims + 1;
                if (item.uDataType != QCBOR_TYPE_BYTE_STRING)
                {
                    LOG(ERROR, "\tRealm personalization value is not in expected format.", 0, 0);
                    return VAL_ERROR;
                }
                if (item.val.string.len != CCA_BYTE_SIZE_64)
                {
                    LOG(ERROR, "\tRealm personalization value size is incorrect.", 0, 0);
                    return VAL_ERROR;
                }
                attestation_token->rpv = item.val.string;
            }
            else if (item.label.int64 == CCA_REALM_INITIAL_MEASUREMENT)
            {
                mandatory_realm_claims = mandatory_realm_claims + 1;
                if (item.uDataType != QCBOR_TYPE_BYTE_STRING)
                {
                    LOG(ERROR, "\tRealm initial measurement is not in expected format.", 0, 0);
                    return VAL_ERROR;
                }
                if (item.val.string.len != CCA_BYTE_SIZE_32 &&
                    item.val.string.len != CCA_BYTE_SIZE_48 &&
                    item.val.string.len != CCA_BYTE_SIZE_64)
                {
                    LOG(ERROR, "\tRealm initial measurement is not in given size format.", 0, 0);
                    return VAL_ERROR;
                }
                attestation_token->realm_initial_measurement = item.val.string;
            }
            else if (item.label.int64 == CCA_REALM_PUBLIC_KEY)
            {
                mandatory_realm_claims = mandatory_realm_claims + 1;
                if (item.uDataType != QCBOR_TYPE_BYTE_STRING)
                {
                    LOG(ERROR, "\tRealm public key is not in expected format.", 0, 0);
                    return VAL_ERROR;
                }
                if (item.val.string.len != CCA_BYTE_SIZE_97)
                {
                    LOG(ERROR, "\tRealm public key size is incorrect.", 0, 0);
                    return VAL_ERROR;
                }
            }
            else if (item.label.int64 == CCA_REALM_HASH_ALGO_ID ||
                     item.label.int64 == CCA_REALM_PUBLIC_KEY_HASH_ALGO_ID)
            {
                mandatory_realm_claims = mandatory_realm_claims + 1;
                if (item.uDataType != QCBOR_TYPE_TEXT_STRING)
                {
                    LOG(ERROR, "\tRealm hash algo/public key hash algo id is \
                                            not in expected format.", 0, 0);
                    return VAL_ERROR;
                }
            }
            else if (item.label.int64 == CCA_REALM_EXTENSIBLE_MEASUREMENT)
            {
                mandatory_realm_claims = mandatory_realm_claims + 1;
                if (item.uDataType != QCBOR_TYPE_ARRAY)
                {
                    LOG(ERROR, "\tRealm extensible measurement is not in expected format.", 0, 0);
                    return VAL_ERROR;
                }

                count = item.val.uCount;
                for (i = 0 ; i < count ; i++)
                {
                    status = QCBORDecode_GetNext(decode_context, &item);
                    attestation_token->rem[i] = item.val.string;
                }
            }
        }
        else
        {
            /* For other claim types */
        }
    }

    if (status == QCBOR_ERR_HIT_END || status == QCBOR_ERR_NO_MORE_ITEMS)
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
                                            QCBORDecodeContext *decode_context, QCBORItem item)
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
                mandatory_platform_claims = mandatory_platform_claims + 1;

                if (item.uDataType != QCBOR_TYPE_TEXT_STRING)
                {
                    LOG(ERROR, "\tPlatform token profile/verification-service/hash algo is \
                                                         not in expected format.", 0, 0);
                    return VAL_ERROR;
                }
            }
            else if (item.label.int64 == CCA_PLATFORM_CHALLENGE)
            {
                mandatory_platform_claims = mandatory_platform_claims + 1;

                if (item.uDataType != QCBOR_TYPE_BYTE_STRING)
                {
                    LOG(ERROR, "\tPlatform challenge is not in expected format.", 0, 0);
                    return VAL_ERROR;
                }
                if (item.val.string.len != CCA_BYTE_SIZE_32 &&
                    item.val.string.len != CCA_BYTE_SIZE_48 &&
                    item.val.string.len != CCA_BYTE_SIZE_64)
                {
                    LOG(ERROR, "\tPlatform challenge size is incorrect.", 0, 0);
                    return VAL_ERROR;
                }
                attestation_token->platform_attest_challenge = item.val.string;
            }
            else if (item.label.int64 == CCA_PLATFORM_IMPLEMENTATION_ID)
            {
                mandatory_platform_claims = mandatory_platform_claims + 1;
                if (item.uDataType != QCBOR_TYPE_BYTE_STRING)
                {
                    LOG(ERROR, "\tPlatform implementation id is not in expected format.", 0, 0);
                    return VAL_ERROR;
                }
                if (item.val.string.len != CCA_BYTE_SIZE_32)
                {
                    LOG(ERROR, "\tPlatform implementation id size is incorrect.", 0, 0);
                    return VAL_ERROR;
                }
            }
            else if (item.label.int64 == CCA_PLATFORM_INSTANCE_ID)
            {
                mandatory_platform_claims = mandatory_platform_claims + 1;
                if (item.uDataType != QCBOR_TYPE_BYTE_STRING)
                {
                    LOG(ERROR, "\tPlatform instance id is not in expected format.", 0, 0);
                    return VAL_ERROR;
                }
                if (item.val.string.len != CCA_BYTE_SIZE_33)
                {
                    LOG(ERROR, "\tPlatform instance id size is incorrect.", 0, 0);
                    return VAL_ERROR;
                }
            }
            else if (item.label.int64 == CCA_PLATFORM_CONFIG)
            {
                mandatory_platform_claims = mandatory_platform_claims + 1;
                if (item.uDataType != QCBOR_TYPE_BYTE_STRING)
                {
                    LOG(ERROR, "\tPlatform config is not in expected format.", 0, 0);
                    return VAL_ERROR;
                }            }
            else if (item.label.int64 == CCA_PLATFORM_LIFESTYLE)
            {
                mandatory_platform_claims = mandatory_platform_claims + 1;
                if (item.uDataType != QCBOR_TYPE_INT64)
                {
                    LOG(ERROR, "\tPlatform lifestyle is not in expected format.", 0, 0);
                    return VAL_ERROR;
                }            }
            else if (item.label.int64 == CCA_PLATFORM_SW_COMPONENTS)
            {
                mandatory_platform_claims = mandatory_platform_claims + 1;
                if (item.uDataType != QCBOR_TYPE_ARRAY)
                {
                    LOG(ERROR, "\tSoftware components is not in expected format.", 0, 0);
                    return VAL_ERROR;
                }
                sw_comp_count = item.val.uCount;
                for (index = 0; index < sw_comp_count; index++)
                {
                    status = QCBORDecode_GetNext(decode_context, &item);
                    if (status != VAL_SUCCESS)
                       continue;

                    count = item.val.uCount;
                    for (i = 0; i <= count; i++)
                    {
                        if (item.label.int64 == CCA_PLATFORM_SW_COMPONENT_TYPE ||
                            item.label.int64 == CCA_PLATFORM_SW_COMPONENT_VERSION ||
                            item.label.int64 == CCA_PLATFORM_SW_COMPONENT_ALGORITHM_ID)
                        {
                            mandaroty_sw_components = mandaroty_sw_components + 1;
                            if (item.uDataType != QCBOR_TYPE_TEXT_STRING)
                            {
                                LOG(ERROR, "\tSoftware component type/version/algorithm is \
                                                         not in expected format.", 0, 0);
                                return VAL_ERROR;
                            }
                        }
                        else if (item.label.int64 == CCA_PLATFORM_SW_COMPONENT_MEASUREMENT_VALUE ||
                                 item.label.int64 == CCA_PLATFORM_SW_COMPONENT_SIGNER_ID)
                        {
                            mandaroty_sw_components = mandaroty_sw_components + 1;
                            if (item.uDataType != QCBOR_TYPE_BYTE_STRING)
                            {
                                LOG(ERROR, "\tSoftware component measurement value/signer is \
                                                            not in expected format.", 0, 0);
                                return VAL_ERROR;
                            }
                        }
                        else if (item.label.int64 == CCA_PLATFORM_LIFESTYLE)
                        {
                            mandaroty_sw_components = mandaroty_sw_components + 1;
                            if (item.uDataType != QCBOR_TYPE_INT64)
                            {
                                LOG(ERROR, "\tPlatform lifstyle is not in expected format.", 0, 0);
                                return VAL_ERROR;
                            }
                        }

                        if (i < count)
                        {
                            status = QCBORDecode_GetNext(decode_context, &item);
                            if (status != VAL_SUCCESS)
                            {
                                LOG(ERROR, "\tNo next software components available.", 0, 0);
                                return VAL_ERROR;
                            }
                        }
                    } /*for (i = 0; i <= count; i++)*/
                } /*for (index = 0; index<sw_comp_count; index++)*/
            }
        }
        else
        {
            /* For other claim types */
        }
    }

    if (status == QCBOR_ERR_HIT_END || status == QCBOR_ERR_NO_MORE_ITEMS)
        return VAL_SUCCESS;
    else
    {
        LOG(ERROR, "\tRealm challenge is not in expected format.", 0, 0);
        return VAL_ERROR;
    }
}

uint64_t val_attestation_verify_token(attestation_token_ts *attestation_token,
            uint64_t *challenge, __attribute__((unused)) size_t challenge_size,
                                        uint8_t *token, size_t token_size)
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

    /* Construct the token buffer for validation */
    completed_token.ptr = token;
    completed_token.len = token_size;

    /* Construct the challenge buffer for validation */
    completed_challenge.ptr = challenge;
    completed_challenge.len = challenge_size;

/*
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
    QCBORDecode_GetNext(&decode_context, &item);

    /* Check the CBOR Map type. Check if the count is 2.
     * Only COSE_SIGN1 is supported now.
     */
    if (item.uDataType != QCBOR_TYPE_MAP || item.val.uCount != 2 ||
        !QCBORDecode_IsTagged(&decode_context, &item, 399))
    {
        LOG(ERROR, "\t Attestation token error formatting", 0, 0);
        return VAL_ERROR;
    }

    /* Get the cca-platform token payload */
    QCBORDecode_GetNext(&decode_context, &item);
    if (item.uDataType != QCBOR_TYPE_BYTE_STRING || item.label.int64 != CCA_PLATFORM_TOKEN)
    {
        LOG(ERROR, "\t Attestation token error formatting", 0, 0);
        return VAL_ERROR;
    }

    platform_token_payload = item.val.string;

    /* Get the realm token payload */
    QCBORDecode_GetNext(&decode_context, &item);
    if (item.uDataType != QCBOR_TYPE_BYTE_STRING  || item.label.int64 != CCA_REALM_TOKEN)
    {
        LOG(ERROR, "\t Attestation token error formatting", 0, 0);
        return VAL_ERROR;
    }

    realm_token_payload = item.val.string;

    /* Initialize the Decoder and validate the cca-platform token payload format */
    QCBORDecode_Init(&decode_context, platform_token_payload, QCBOR_DECODE_MODE_NORMAL);
    status = QCBORDecode_GetNext(&decode_context, &item);
    if (status != VAL_SUCCESS)
        return status;

    if (item.uDataType != QCBOR_TYPE_ARRAY || item.val.uCount != 4 ||
            !QCBORDecode_IsTagged(&decode_context, &item, CBOR_TAG_COSE_SIGN1))
    {
        LOG(ERROR, "\t Attestation token error formatting", 0, 0);
        return VAL_ERROR;
    }
    status = QCBORDecode_GetNext(&decode_context, &item);
    status = QCBORDecode_GetNext(&decode_context, &item);
    status = QCBORDecode_GetNext(&decode_context, &item);

    payload1 = item.val.string;

    /* Verify the signature */
    status = pal_verify_signature(token);
    if (status != VAL_SUCCESS)
        return status;

    QCBORDecode_Init(&decode_context, payload1, QCBOR_DECODE_MODE_NORMAL);
    status = QCBORDecode_GetNext(&decode_context, &item);
    if (item.uDataType != QCBOR_TYPE_MAP)
    {
        LOG(ERROR, "\t Attestation token error formatting", 0, 0);
        return VAL_ERROR;
    }
    /* Parse the payload and check the data type of each claim */
    status = parse_claims_platform_token(attestation_token, &decode_context, item);
    if (status != VAL_SUCCESS)
        return status;

    /* Initialize the Decoder and validate the realm token payload format */
    QCBORDecode_Init(&decode_context, realm_token_payload, QCBOR_DECODE_MODE_NORMAL);
    status = QCBORDecode_GetNext(&decode_context, &item);
    if (status != VAL_SUCCESS)
        return status;

    if (item.uDataType != QCBOR_TYPE_ARRAY || item.val.uCount != 4 ||
            !QCBORDecode_IsTagged(&decode_context, &item, CBOR_TAG_COSE_SIGN1))
    {
        LOG(ERROR, "\t Attestation token error formatting", 0, 0);
        return VAL_ERROR;
    }

    status = QCBORDecode_GetNext(&decode_context, &item);
    status = QCBORDecode_GetNext(&decode_context, &item);
    status = QCBORDecode_GetNext(&decode_context, &item);

    payload2 = item.val.string;
    QCBORDecode_Init(&decode_context, payload2, QCBOR_DECODE_MODE_NORMAL);


    status = QCBORDecode_GetNext(&decode_context, &item);
    if (item.uDataType != QCBOR_TYPE_MAP)
    {
        LOG(ERROR, "\t Attestation token error formatting", 0, 0);
        return VAL_ERROR;
    }

    /* Parse the payload and check the data type of each claim */
    status = parse_claims_realm_token(attestation_token, &decode_context, item,
                                                          completed_challenge);
    if (status != VAL_SUCCESS)
        return status;

    if (mandatory_realm_claims != 7)
    {
        LOG(ERROR, "\t mandatory realm claims are not there", 0, 0);
        return VAL_ERROR;
    }

    if (!(mandatory_platform_claims >= 8) && !(mandaroty_sw_components >= 2))
    {
        LOG(ERROR, "\t mandatory platform claims/sw_components are not there.", 0, 0);
        return VAL_ERROR;
    }
    return VAL_SUCCESS;
}
