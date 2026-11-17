/*
 * Copyright (c) 2023, 2025-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"

#define HASH_SHA_512_FEATURE_BIT 5U
#define MAX_TEST_REALMS          2U

void cmd_measurement_extend_host(void)
{
    val_host_realm_ts realm[MAX_TEST_REALMS];
    const uint8_t hash_algorithm[MAX_TEST_REALMS] = {
        RMI_HASH_SHA_256,
        RMI_HASH_SHA_512
    };
    uint64_t *shared_hash_algorithm =
        (uint64_t *)(val_get_shared_region_base() + TEST_USE_OFFSET1);
    uint64_t feature_reg;
    uint16_t realm_count = 1U;
    uint64_t ret;
    uint16_t i;

    if (val_host_rmi_features(RMI_FEATURE_REGISTER_1_INDEX, &feature_reg))
    {
        LOG(ERROR, "RMI_FEATURES(1) failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    if (VAL_EXTRACT_BITS(feature_reg, HASH_SHA_512_FEATURE_BIT,
                         HASH_SHA_512_FEATURE_BIT) == RMI_FEATURE_TRUE)
        realm_count = MAX_TEST_REALMS;

    for (i = 0; i < realm_count; i++)
    {
        val_memset(&realm[i], 0, sizeof(realm[i]));
        val_host_realm_params(&realm[i]);
        realm[i].hash_algo = hash_algorithm[i];
        realm[i].vmid = i;

        if (val_host_realm_setup(&realm[i], 1))
        {
            LOG(ERROR, "Realm setup failed\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto destroy_realm;
        }

        *shared_hash_algorithm = hash_algorithm[i];
        ret = val_host_rmi_rec_enter(realm[i].rec[0], realm[i].run[0]);
        if (ret)
        {
            LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_realm;
        }

        if (IS_TEST_FAIL(val_get_status()))
            goto destroy_realm;
    }

    if (IS_TEST_START(val_get_status()))
        val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}
