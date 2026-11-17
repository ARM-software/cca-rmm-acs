/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_version_data.h"

#define VER(maj, min) ((((uint64_t)(maj)) << 16) | ((uint64_t)(min)))
#define RMI_VERSION_MAX VER(0x7FFFU, 0xFFFFU)

struct arguments {
    uint64_t req;
};

static uint64_t reported_higher_version(void)
{
    val_host_rmi_version_ts rmi_ver_out;
    uint64_t expected_version;
    uint64_t ret;

    expected_version = VER(RMI_ABI_VERSION_MAJOR, RMI_ABI_VERSION_MINOR);
    ret = val_host_rmi_version(expected_version, &rmi_ver_out);
    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "RMI_VERSION preparation failed, ret=0x%lx\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return rmi_ver_out.higher;
}

static uint64_t incompat_lower_prep_sequence(struct arguments *args)
{
    uint64_t higher_version;

    higher_version = reported_higher_version();
    if (higher_version == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    if (higher_version >= RMI_VERSION_MAX) {
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

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case INCOMPAT_LOWER:
            if (incompat_lower_prep_sequence(args))
                return VAL_ERROR;
            break;

        case INCOMPAT_HIGHER:
            if (incompat_higher_prep_sequence(args))
                return VAL_ERROR;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_rmi_version_host(void)
{
    val_host_rmi_version_ts rmi_ver_out;
    uint64_t expected_version;
    uint64_t ret, i;
    struct arguments args;

    expected_version = VER(RMI_ABI_VERSION_MAJOR, RMI_ABI_VERSION_MINOR);

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
              i + 1, test_data[i].msg, test_data[i].label);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto exit;
        }

        ret = val_host_rmi_version(args.req, &rmi_ver_out);
        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "%s failed: ret=0x%lx\n", test_data[i].msg, ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        if ((VAL_EXTRACT_BITS(rmi_ver_out.lower, 31, 63) != 0) ||
            (VAL_EXTRACT_BITS(rmi_ver_out.higher, 31, 63) != 0) ||
            (rmi_ver_out.lower > rmi_ver_out.higher)) {
            LOG(ERROR, "%s returned an invalid version range\n",
                test_data[i].msg);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }

        if ((test_data[i].label == INCOMPAT_LOWER) &&
            (rmi_ver_out.lower >= args.req)) {
            LOG(ERROR, "incompat_lower did not return a lower revision\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }

        if ((test_data[i].label == INCOMPAT_HIGHER) &&
            (rmi_ver_out.lower != rmi_ver_out.higher)) {
            LOG(ERROR, "incompat_higher returned different lower and higher revisions\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);

    ret = val_host_rmi_version(expected_version, &rmi_ver_out);
    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "Expected RMI_SUCCESS, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    /* RmiInterfaceVersion[63:31] is MBZ. */
    if ((VAL_EXTRACT_BITS(rmi_ver_out.lower, 31, 63) != 0) ||
        (VAL_EXTRACT_BITS(rmi_ver_out.higher, 31, 63) != 0)) {
        LOG(ERROR, "Received non-zero reserved version field\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    /* Success condition: lower is equal to the requested version. */
    if (rmi_ver_out.lower != expected_version) {
        LOG(ERROR, "Unexpected lower version: 0x%lx, expected: 0x%lx\n",
            rmi_ver_out.lower, expected_version);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    if (rmi_ver_out.higher < rmi_ver_out.lower) {
        LOG(ERROR, "Higher version 0x%lx is below lower version 0x%lx\n",
            rmi_ver_out.higher, rmi_ver_out.lower);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    return;
}
