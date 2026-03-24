/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_pdev_set_pubkey_data.h"
#include "command_common_host.h"
#include "val_host_realm.h"
#include "val_host_command.h"
#include "val_host_pcie.h"
#include "val_host_doe.h"
#include "pal_common_support.h"
#include "val_timer.h"
#include "val_crypto.h"
#include "mbedtls/debug.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/oid.h"
#include "val_host_helpers.h"

static uint32_t num_bdf;

static struct argument_store {
    uint64_t pdev_ptr_valid;
    uint64_t params_ptr_valid;
    val_host_pdev_ts pdev_dev_valid;
    uint8_t key[1024];
    uint8_t metadata[1024];
    uint64_t key_len;
    uint64_t metadata_len;
    uint8_t algo;
} c_args;

static struct argument {
    uint64_t pdev_new_invalid;
    val_host_pdev_ts pdev_dev_invalid;
} c_args_invalid;

struct arguments {
    uint64_t pdev_ptr;
    uint64_t params_ptr;
};

static uint64_t invalid_key_len_prep_sequence(void)
{
    val_host_public_key_params_ts *pubkey_params;

    pubkey_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!pubkey_params)
    {
        LOG(ERROR, "\tFailed to allocate memory for pub_key\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    val_memset(pubkey_params, 0, PAGE_SIZE_4K);

    val_memcpy(pubkey_params->key, &(c_args.key), c_args.key_len);
    pubkey_params->key_len = MAX_PUB_KEY_LEN + 1;
    pubkey_params->metadata_len = c_args.metadata_len;
    pubkey_params->algo = c_args.algo;
    val_memcpy(pubkey_params->metadata, &c_args.metadata, c_args.metadata_len);

    return (uint64_t)pubkey_params;
}

static uint64_t invalid_metadata_len_prep_sequence(void)
{
    val_host_public_key_params_ts *pubkey_params;

    pubkey_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!pubkey_params)
    {
        LOG(ERROR, "\tFailed to allocate memory for pub_key\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    val_memset(pubkey_params, 0, PAGE_SIZE_4K);

    val_memcpy(pubkey_params->key, &(c_args.key), c_args.key_len);
    pubkey_params->key_len = c_args.key_len;
    pubkey_params->metadata_len = MAX_METADATA_LEN + 1;
    pubkey_params->algo = c_args.algo;
    val_memcpy(pubkey_params->metadata, &c_args.metadata, c_args.metadata_len);

    return (uint64_t)pubkey_params;
}

static uint64_t invalid_key_prep_sequence(void)
{
    val_host_public_key_params_ts *pubkey_params;

    pubkey_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!pubkey_params)
    {
        LOG(ERROR, "\tFailed to allocate memory for pub_key\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    val_memset(pubkey_params, 0, PAGE_SIZE_4K);

    val_memcpy(pubkey_params->key, &(c_args.key), c_args.key_len);
    pubkey_params->key_len = c_args.key_len + 5;
    pubkey_params->metadata_len = c_args.metadata_len;
    pubkey_params->algo = c_args.algo;
    val_memcpy(pubkey_params->metadata, &c_args.metadata, c_args.metadata_len);

    return (uint64_t)pubkey_params;
}

static uint64_t invalid_metadata_prep_sequence(void)
{
    val_host_public_key_params_ts *pubkey_params;

    pubkey_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!pubkey_params)
    {
        LOG(ERROR, "\tFailed to allocate memory for pub_key\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    val_memset(pubkey_params, 0, PAGE_SIZE_4K);

    val_memcpy(pubkey_params->key, &(c_args.key), c_args.key_len);
    pubkey_params->key_len = c_args.key_len;
    pubkey_params->metadata_len = c_args.metadata_len + 5;
    pubkey_params->algo = c_args.algo;
    val_memcpy(pubkey_params->metadata, &c_args.metadata, c_args.metadata_len);

    return (uint64_t)pubkey_params;
}

static uint64_t pdev_new_prep_sequence(void)
{
    val_host_pdev_ts pdev_dev;
    val_host_pdev_flags_ts pdev_flags;
    uint32_t rp_bdf, status;

    val_memset(&pdev_dev, 0, sizeof(pdev_dev));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

    if (val_pcie_find_doe_capability(&num_bdf, (uint32_t *)&pdev_dev.bdf,
                                                     &pdev_dev.doe_cap_base))
        return VAL_TEST_PREP_SEQ_FAILED;

    status = val_pcie_get_rootport((uint32_t)pdev_dev.bdf, &rp_bdf);
    if (status != 0)
        return VAL_TEST_PREP_SEQ_FAILED;

    pdev_dev.root_id = (uint16_t)rp_bdf;

    val_host_set_pdev_flags(&pdev_flags);
    val_memcpy(&pdev_dev.pdev_flags, &pdev_flags, sizeof(pdev_flags));

    pdev_dev.pdev = g_pdev_new_prep_sequence(&pdev_dev);
    if (c_args.pdev_ptr_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args_invalid.pdev_new_invalid = pdev_dev.pdev;
    c_args_invalid.pdev_dev_invalid = pdev_dev;

    return pdev_dev.pdev;
}
static uint64_t valid_input_args_prep_sequence(void)
{
    uint64_t pdev, req_addr, resp_addr;
    val_host_pdev_ts pdev_dev;
    val_host_pdev_flags_ts pdev_flags;
    uint32_t status, rp_bdf;
    val_host_public_key_params_ts *pubkey_params;
    int rc;
    uint8_t public_key_algo;

    val_memset(&pdev_dev, 0, sizeof(pdev_dev));
    val_memset(&pdev_flags, 0, sizeof(pdev_flags));

    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR)
        return VAL_TEST_PREP_SEQ_FAILED;

    if (val_pcie_find_doe_capability(&num_bdf, (uint32_t *)&pdev_dev.bdf,
                                                     &pdev_dev.doe_cap_base))
        return VAL_TEST_PREP_SEQ_FAILED;

    status = val_pcie_get_rootport((uint32_t)pdev_dev.bdf, &rp_bdf);
    if (status != 0)
        return VAL_TEST_PREP_SEQ_FAILED;

    pdev_dev.root_id = (uint16_t)rp_bdf;

    val_host_set_pdev_flags(&pdev_flags);
    val_memcpy(&pdev_dev.pdev_flags, &pdev_flags, sizeof(pdev_flags));

    pdev = g_pdev_new_prep_sequence(&pdev_dev);
    if (pdev == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    /* Allocate memory for req addr buffer */
    req_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!req_addr)
    {
        LOG(ERROR, "Failed to allocate memory for req_addr.\n");
        goto exit;
    }

    /* Allocate memory for resp_addr buffer */
    resp_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!resp_addr)
    {
        LOG(ERROR, "Failed to allocate memory for resp_addr.\n");
        goto exit;
    }

    /* Allocate buffer to cache VCA */
    pdev_dev.vca = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_PDEV_VCA_LEN_MAX);
    pdev_dev.vca_len = 0;
    if (pdev_dev.vca == NULL) {
        goto exit;
    }

    /* Allocate buffer to cache device certificate */
    pdev_dev.cert_slot_id = 0;
    pdev_dev.cert_chain = val_host_mem_alloc(PAGE_SIZE, VAL_HOST_PDEV_CERT_LEN_MAX);
    pdev_dev.cert_chain_len = 0;
    if (pdev_dev.cert_chain == NULL) {
        goto exit;
    }

    /* Allocate buffer to store extracted public key */
    pdev_dev.public_key = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev_dev.public_key == NULL) {
        goto exit;
    }
    pdev_dev.public_key_len = PAGE_SIZE;

    /* Allocate buffer to store public key metadata */
    pdev_dev.public_key_metadata = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (pdev_dev.public_key_metadata == NULL) {
        goto exit;
    }
    pdev_dev.public_key_metadata_len = PAGE_SIZE;

    pdev_dev.pdev = pdev;
    pdev_dev.dev_comm_data = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    pdev_dev.dev_comm_data->enter.req_addr = req_addr;
    pdev_dev.dev_comm_data->enter.resp_addr = resp_addr;

    if (val_host_dev_communicate(NULL, &pdev_dev, NULL, RMI_PDEV_NEEDS_KEY))
    {
        LOG(ERROR, "PDEV communicate failed.\n");
        goto exit;
    }

    if (val_host_check_pdev_state(pdev, RMI_PDEV_NEEDS_KEY))
    {
        LOG(ERROR, "PDEV state check failed.\n");
        goto exit;
    }

    LOG(TEST, "PDEV State is changed to PDEV_NEEDS_KEY\n");

    /* Get public key */
    rc = val_host_get_public_key_from_cert_chain(pdev_dev.cert_chain,
                         pdev_dev.cert_chain_len,
                         pdev_dev.public_key,
                         &pdev_dev.public_key_len,
                         pdev_dev.public_key_metadata,
                         &pdev_dev.public_key_metadata_len,
                         &public_key_algo);
    if (rc != 0) {
        LOG(ERROR, "Get public key failed\n");
        goto exit;
    }

    if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P256) {
        pdev_dev.public_key_sig_algo = RMI_SIG_ECDSA_P256;
    } else if (public_key_algo == PUBLIC_KEY_ALGO_ECDSA_ECC_NIST_P384) {
        pdev_dev.public_key_sig_algo = RMI_SIG_ECDSA_P384;
    } else {
        pdev_dev.public_key_sig_algo = RMI_SIG_RSASSA_3072;
    }

    pubkey_params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!pubkey_params)
    {
        LOG(ERROR, "Failed to allocate memory for pub_key\n");
        goto exit;
    }

    val_memset(pubkey_params, 0, PAGE_SIZE_4K);
    val_memcpy(pubkey_params->key, pdev_dev.public_key, pdev_dev.public_key_len);
    val_memcpy(pubkey_params->metadata, pdev_dev.public_key_metadata,
                                pdev_dev.public_key_metadata_len);
    pubkey_params->key_len = pdev_dev.public_key_len;
    pubkey_params->metadata_len = pdev_dev.public_key_metadata_len;
    pubkey_params->algo = pdev_dev.public_key_sig_algo;

    c_args.key_len = pubkey_params->key_len;
    c_args.metadata_len = pubkey_params->metadata_len;
    c_args.algo = pubkey_params->algo;

    val_memcpy(&c_args.metadata, pubkey_params->metadata, c_args.metadata_len);
    val_memcpy(c_args.key, pubkey_params->key, pubkey_params->key_len);

    c_args.params_ptr_valid = (uint64_t)pubkey_params;
    c_args.pdev_ptr_valid = pdev_dev.pdev;
    c_args.pdev_dev_valid = pdev_dev;

    return VAL_SUCCESS;
exit:
    return VAL_TEST_PREP_SEQ_FAILED;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case PDEV_UNALIGNED:
            args->pdev_ptr = g_unaligned_prep_sequence(c_args.pdev_ptr_valid);
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case PDEV_OUTSIDE_OF_PERMITTED_PA:
            args->pdev_ptr = g_outside_of_permitted_pa_prep_sequence();
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case PDEV_DEV_MEM_MMIO:
            args->pdev_ptr = g_dev_mem_prep_sequence();
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case PDEV_GRAN_STATE_UNDELEGATED:
            args->pdev_ptr = g_undelegated_prep_sequence();
            args->params_ptr = c_args.params_ptr_valid;
            break;

        case PARAMS_UNALIGNED:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = g_unaligned_prep_sequence(c_args.params_ptr_valid);
            break;

        case PARAMS_PAS_REALM:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = g_delegated_prep_sequence();
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case PARAMS_PAS_SECURE:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = g_secure_prep_sequence();
            break;

        case INVALID_KEY_LEN:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = invalid_key_len_prep_sequence();
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case INVALID_METADATA_LEN:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = invalid_metadata_len_prep_sequence();
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case INVALID_KEY:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = invalid_key_prep_sequence();
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case INVALID_METADATA:
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->params_ptr = invalid_metadata_prep_sequence();
            if (args->params_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case PDEV_STATE_PDEV_NEW:
            args->pdev_ptr = pdev_new_prep_sequence();
            if (args->pdev_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_TEST_PREP_SEQ_FAILED;
            args->params_ptr = c_args.params_ptr_valid;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_pdev_set_pubkey_host(void)
{
    uint64_t i;
    val_smc_param_ts cmd_ret;
    struct arguments args;

    /* Skip if RMM do not support DA */
    if (!val_host_rmm_supports_da())
    {
        LOG(ALWAYS, "DA feature not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto exit;
    }

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_device;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "\n\tCheck %d : ", i + 1);
        LOG(TEST, test_data[i].msg);
        LOG(TEST, "; intent id : 0x%x \n", test_data[i].label);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto destroy_device;
        }

        cmd_ret = val_host_rmi_pdev_set_pubkey(args.pdev_ptr, args.params_ptr);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_device;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n");
    cmd_ret = val_host_rmi_pdev_set_pubkey(c_args.pdev_ptr_valid, c_args.params_ptr_valid);
    if (cmd_ret.x0 != 0)
    {
        LOG(ERROR, "Command failed. %x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    /* PDEV state should be PDEV_HAS_KEY */
    if (val_host_check_pdev_state(c_args.pdev_ptr_valid, RMI_PDEV_HAS_KEY))
    {
        LOG(ERROR, "PDEV state check failed.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_device;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    if (val_host_pdev_teardown(&c_args.pdev_dev_valid, c_args.pdev_ptr_valid))
    {
        LOG(ERROR, "PDEV teardown failed.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    if (val_host_pdev_teardown(&c_args_invalid.pdev_dev_invalid, c_args_invalid.pdev_new_invalid))
    {
        LOG(ERROR, "PDEV teardown failed.\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

exit:
    return;
}
