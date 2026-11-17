/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_realm_framework.h"
#include "rsi_version_data.h"

#define VER(maj, min) ((((uint64_t)(maj)) << 16) | ((uint64_t)(min)))
#define RSI_VERSION_MAX VER(0x7FFFU, 0xFFFFU)

struct arguments {
    uint64_t req;
};

static uint64_t reported_higher_version(void)
{
    val_realm_rsi_version_ts rsi_ver_out;
    uint64_t ret;

    ret = val_realm_rsi_version(VER(RSI_ABI_VERSION_MAJOR,
                                    RSI_ABI_VERSION_MINOR),
                                &rsi_ver_out);
    if (ret != RSI_SUCCESS) {
        LOG(ERROR, "RSI_VERSION preparation failed, ret=0x%lx\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return rsi_ver_out.higher;
}

static uint64_t incompat_lower_prep_sequence(struct arguments *args)
{
    uint64_t higher_version = reported_higher_version();

    if (higher_version == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    if (higher_version >= RSI_VERSION_MAX) {
        LOG(ERROR, "No interface version exists above the reported highest version\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    args->req = higher_version + 1;

    return VAL_SUCCESS;
}

static uint64_t incompat_higher_prep_sequence(struct arguments *args)
{
    args->req = VER(0, 0);

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test, struct arguments *args)
{
    switch (test->label) {
    case INCOMPAT_LOWER:
        return incompat_lower_prep_sequence(args);
    case INCOMPAT_HIGHER:
        return incompat_higher_prep_sequence(args);
    default:
        LOG(ERROR, "Unknown intent label encountered\n");
        return VAL_ERROR;
    }
}

void cmd_rsi_version_realm(void)
{
    val_realm_rsi_version_ts rsi_ver_out;
    struct arguments args;
    uint64_t expected_version;
    uint64_t ret, i;

    expected_version = VER(RSI_ABI_VERSION_MAJOR, RSI_ABI_VERSION_MINOR);

    for (i = 0; i < (sizeof(test_data) / sizeof(test_data[0])); i++) {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x\n",
            i + 1, test_data[i].msg, test_data[i].label);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto exit;
        }

        ret = val_realm_rsi_version(args.req, &rsi_ver_out);
        if (ret != test_data[i].status) {
            LOG(ERROR, "%s failed: ret=0x%lx\n", test_data[i].msg, ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        if ((VAL_EXTRACT_BITS(rsi_ver_out.lower, 31, 63) != 0) ||
            (VAL_EXTRACT_BITS(rsi_ver_out.higher, 31, 63) != 0) ||
            (rsi_ver_out.lower > rsi_ver_out.higher)) {
            LOG(ERROR, "%s returned an invalid version range\n",
                test_data[i].msg);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }

        if ((test_data[i].label == INCOMPAT_LOWER) &&
            (rsi_ver_out.lower >= args.req)) {
            LOG(ERROR, "incompat_lower did not return a lower revision\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }

        if ((test_data[i].label == INCOMPAT_HIGHER) &&
            (rsi_ver_out.lower != rsi_ver_out.higher)) {
            LOG(ERROR, "incompat_higher returned different lower and higher revisions\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);

    ret = val_realm_rsi_version(expected_version, &rsi_ver_out);
    if (ret != RSI_SUCCESS) {
        LOG(ERROR, "Expected RSI_SUCCESS, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    if ((VAL_EXTRACT_BITS(rsi_ver_out.lower, 31, 63) != 0) ||
        (VAL_EXTRACT_BITS(rsi_ver_out.higher, 31, 63) != 0)) {
        LOG(ERROR, "Received non-zero reserved version field\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    if (rsi_ver_out.lower != expected_version) {
        LOG(ERROR, "Unexpected lower version: 0x%lx, expected: 0x%lx\n",
            rsi_ver_out.lower, expected_version);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    if (rsi_ver_out.higher < rsi_ver_out.lower) {
        LOG(ERROR, "Higher version 0x%lx is below lower version 0x%lx\n",
            rsi_ver_out.higher, rsi_ver_out.lower);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    val_realm_return_to_host();
}
