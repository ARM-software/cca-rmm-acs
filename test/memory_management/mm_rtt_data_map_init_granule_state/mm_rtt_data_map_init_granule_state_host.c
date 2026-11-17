/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_realm.h"
#include "val_host_rmi.h"
#include "val_host_rmi_sro.h"

#define DATA_MAP_INIT_IPA 0x900000
#define MAP_LEVEL         3

static uint64_t check_data_granule_state(uint64_t data)
{
    uint64_t ret;

    ret = val_host_rmi_granule_range_undelegate(data, data + PAGE_SIZE);
    if (RMI_STATUS(ret) != RMI_ERROR_INPUT)
    {
        LOG(ERROR, "Mapped data granule was not rejected by undelegate, ret=0x%lx\n", ret);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t unmap_data(uint64_t rd, uint64_t ipa)
{
    val_host_rmi_rtt_unmap_flags_ts unmap_flags = {0};
    uint64_t flags = 0;

    unmap_flags.oaddr_type = RMI_ADDR_TYPE_SINGLE;
    val_memcpy(&flags, &unmap_flags, sizeof(unmap_flags));

    return val_host_rmi_rtt_data_unmap(rd, ipa, ipa + PAGE_SIZE, flags, 0);
}

void mm_rtt_data_map_init_granule_state_host(void)
{
    val_host_realm_ts realm;
    val_host_rtt_entry_ts rtte;
    uint64_t data;
    uint64_t src;
    uint64_t ret;
    bool data_delegated = false;
    bool data_mapped = false;

    val_host_realm_params(&realm);

    if (val_host_realm_create(&realm))
    {
        LOG(ERROR, "Realm create failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    if (val_host_ripas_init(&realm, DATA_MAP_INIT_IPA,
                            DATA_MAP_INIT_IPA + PAGE_SIZE,
                            VAL_RTT_MAX_LEVEL, PAGE_SIZE))
    {
        LOG(ERROR, "Couldn't create protected mapping\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        return;
    }

    data = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    src = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((data == 0) || (src == 0))
    {
        LOG(ERROR, "Failed to allocate DATA_MAP_INIT granules\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        return;
    }

    ret = val_host_rmi_granule_range_delegate(data, data + PAGE_SIZE);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_GRANULE_DELEGATE failed, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        return;
    }
    data_delegated = true;

    ret = val_host_rmi_rtt_data_map_init(realm.rd, data, DATA_MAP_INIT_IPA,
                                         src, RMI_MEASURE_CONTENT);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "RMI_RTT_DATA_MAP_INIT failed, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto cleanup;
    }
    data_delegated = false;
    data_mapped = true;

    ret = val_host_rmi_rtt_read_entry(realm.rd, DATA_MAP_INIT_IPA,
                                      MAP_LEVEL, &rtte);
    if ((ret != RMI_SUCCESS) || (rtte.state != RMI_ASSIGNED) ||
        (OA(rtte.desc) != data) || (rtte.ripas != RMI_RAM))
    {
        LOG(ERROR, "Mapped RTTE does not point to DATA granule, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto cleanup;
    }

    if (check_data_granule_state(data) != VAL_SUCCESS)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto cleanup;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    if (data_mapped)
    {
        ret = unmap_data(realm.rd, DATA_MAP_INIT_IPA);
        if (ret != RMI_SUCCESS)
        {
            LOG(ERROR, "RMI_RTT_DATA_UNMAP cleanup failed, ret=0x%lx\n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
            return;
        }
        data_delegated = true;
    }

    if (data_delegated &&
        (val_host_rmi_granule_range_undelegate(data, data + PAGE_SIZE) != RMI_SUCCESS))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
    }
}
