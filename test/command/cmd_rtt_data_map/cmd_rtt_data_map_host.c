/*
 * Copyright (c) 2023-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "test_database.h"
#include "val_host_realm.h"
#include "val_host_rmi.h"
#include "val_host_rmi_sro.h"
#include "val_host_sro.h"
#include "rmi_rtt_data_map_data.h"
#include "command_common_host.h"
#include "val_host_pcie.h"

#define NUM_REALMS 1
#define VALID_REALM 0
#define MAP_LEVEL 3
#define IPA_VALID 0UL
#define IPA_DATA  (5UL * PAGE_SIZE)
#define IPA_LIST  (7UL * PAGE_SIZE)
#define DATA_TEST_PATTERN 0xA5

static val_host_realm_ts realm[NUM_REALMS];

static struct {
    uint64_t rd;
    uint64_t rtt;
    uint64_t base;
    uint64_t top;
    uint64_t flags;
    uint64_t oaddr;
    uint64_t data;
} valid;

static val_host_tracking_test_region_ts tracking;

struct arguments {
    uint64_t rd;
    uint64_t base;
    uint64_t top;
    uint64_t flags;
    uint64_t oaddr;
};

static uint64_t tracking_prep_sequence(void)
{
    uint64_t ret = val_host_tracking_coarse_prep_sequence(&tracking);

    if (ret == PLATFORM_TRACKING_TEST_REGION_BASE)
        return VAL_SUCCESS;

    return ret;
}

static uint32_t tracking_cleanup(void)
{
    return val_host_tracking_coarse_cleanup(&tracking);
}

static uint64_t prepare_delegated_data(uint64_t *data)
{
    *data = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (*data == 0)
        return VAL_ERROR;

    val_memset((void *)*data, DATA_TEST_PATTERN, PAGE_SIZE);
    if (val_host_rmi_granule_range_delegate(*data, *data + PAGE_SIZE) != RMI_SUCCESS)
        return VAL_ERROR;

    return VAL_SUCCESS;
}

static uint64_t check_data_wiped(uint64_t data)
{
    uint8_t *ptr = (uint8_t *)data;
    uint64_t i;

    for (i = 0; i < PAGE_SIZE; i++) {
        if (ptr[i] != 0) {
            LOG(ERROR, "Mapped DATA was not wiped, offset=0x%x value=0x%x\n",
                i, ptr[i]);
            return VAL_ERROR;
        }
    }

    return VAL_SUCCESS;
}

static uint64_t check_mapped_granule_state(uint64_t data)
{
    uint64_t ret;

    ret = val_host_rmi_granule_range_undelegate(data, data + PAGE_SIZE);
    if (RMI_STATUS(ret) != RMI_ERROR_INPUT) {
        LOG(ERROR, "Mapped Granule did not have DATA state, ret=0x%x\n", ret);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t unmap_and_check_wiped(uint64_t ipa, uint64_t data)
{
    val_host_rmi_rtt_unmap_flags_ts unmap_flags = {0};
    uint64_t flags = 0;
    uint64_t ret;

    unmap_flags.oaddr_type = RMI_ADDR_TYPE_SINGLE;
    val_memcpy(&flags, &unmap_flags, sizeof(unmap_flags));

    ret = val_host_rmi_rtt_data_unmap(valid.rd, ipa, ipa + PAGE_SIZE,
                                      flags, 0);
    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "RMI_RTT_DATA_UNMAP cleanup failed, ret=0x%x\n", ret);
        return VAL_ERROR;
    }

    ret = val_host_rmi_granule_range_undelegate(data, data + PAGE_SIZE);
    if (ret != RMI_SUCCESS) {
        LOG(ERROR, "Mapped DATA did not become DELEGATED, ret=0x%x\n", ret);
        return VAL_ERROR;
    }

    return check_data_wiped(data);
}

static uint64_t check_list_positive(void)
{
    val_host_rmi_rtt_prot_map_flags_ts map_flags = {0};
    val_host_rtt_entry_ts rtte;
    val_smc_param_ts cmd;
    uint64_t *list;
    uint64_t list_addr;
    uint64_t data = 0;
    uint64_t flags = 0;
    uint64_t ret;
    bool mapped = false;

    if (create_mapping(IPA_LIST, true, valid.rd) != 0)
        return VAL_ERROR;

    if (prepare_delegated_data(&data) != VAL_SUCCESS)
        return VAL_ERROR;

    list_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (list_addr == 0) {
        (void)val_host_rmi_granule_range_undelegate(data, data + PAGE_SIZE);
        return VAL_ERROR;
    }

    list = (uint64_t *)list_addr;
    val_memset(list, 0, PAGE_SIZE);
    list[0] = val_host_rmi_addr_range_desc(data, 1, RMI_PAGE_L3,
                                      RMI_OP_MEM_DELEGATED);

    map_flags.oaddr_type = RMI_ADDR_TYPE_LIST;
    map_flags.list_count = 1;
    val_memcpy(&flags, &map_flags, sizeof(map_flags));

    cmd = val_host_rmi_rtt_data_map_smc(valid.rd, IPA_LIST,
                                        IPA_LIST + PAGE_SIZE, flags, list_addr);
    ret = val_host_sro_complete(&cmd);
    if ((ret != RMI_SUCCESS) || (cmd.x1 != IPA_LIST + PAGE_SIZE)) {
        LOG(ERROR, "RMI_RTT_DATA_MAP LIST positive check failed, ret=0x%x\n", ret);
        goto cleanup;
    }

    mapped = true;
    val_host_update_granule_state(valid.rd, GRANULE_DATA, data,
                                  IPA_LIST, 0, 0);

    ret = val_host_rmi_rtt_read_entry(valid.rd, IPA_LIST, MAP_LEVEL, &rtte);
    if ((ret != RMI_SUCCESS) || (rtte.state != RMI_ASSIGNED) ||
        (OA(rtte.desc) != data) || check_mapped_granule_state(data)) {
        LOG(ERROR, "RMI_RTT_DATA_MAP LIST observability failed, ret=0x%x\n", ret);
        ret = VAL_ERROR;
        goto cleanup;
    }

    ret = unmap_and_check_wiped(IPA_LIST, data);
    if (ret == VAL_SUCCESS)
        mapped = false;

cleanup:
    if (mapped)
        (void)unmap_and_check_wiped(IPA_LIST, data);
    else if (ret != VAL_SUCCESS)
        (void)val_host_rmi_granule_range_undelegate(data, data + PAGE_SIZE);

    val_host_mem_free((void *)list_addr);
    return ret;
}

static uint64_t prepare_valid_args(void)
{
    val_host_rmi_rtt_prot_map_flags_ts map_flags = {0};

    val_memset(&realm[VALID_REALM], 0, sizeof(realm[VALID_REALM]));
    realm[VALID_REALM].s2sz = 40;
    realm[VALID_REALM].hash_algo = RMI_HASH_SHA_256;
    realm[VALID_REALM].s2_starting_level = 0;
    realm[VALID_REALM].num_s2_sl_rtts = 1;
    realm[VALID_REALM].vmid = VALID_REALM;

    if (val_host_realm_create_common(&realm[VALID_REALM]) != 0)
        return VAL_TEST_PREP_SEQ_FAILED;

    valid.rd = realm[VALID_REALM].rd;
    valid.rtt = realm[VALID_REALM].rtt_l0_addr;
    if (create_mapping(IPA_VALID, true, valid.rd) != 0)
        return VAL_TEST_PREP_SEQ_FAILED;

    if (prepare_delegated_data(&valid.data) != VAL_SUCCESS)
        return VAL_TEST_PREP_SEQ_FAILED;

    valid.base = IPA_VALID;
    valid.top = IPA_VALID + PAGE_SIZE;
    map_flags.oaddr_type = RMI_ADDR_TYPE_SINGLE;
    val_memcpy(&valid.flags, &map_flags, sizeof(map_flags));
    valid.oaddr = val_host_rmi_addr_range_desc(valid.data, 1, RMI_PAGE_L3,
                                          RMI_OP_MEM_DELEGATED);
    return VAL_SUCCESS;
}

static uint64_t prepare_oaddr_value(struct arguments *args)
{
    uint64_t mapped = g_delegated_prep_sequence();

    if (mapped == VAL_TEST_PREP_SEQ_FAILED ||
        create_mapping(IPA_DATA, true, valid.rd) != 0)
        return VAL_ERROR;

    if (val_host_rmi_rtt_data_map(valid.rd, IPA_DATA,
                                  IPA_DATA + PAGE_SIZE, valid.flags,
                                  val_host_rmi_addr_range_desc(mapped, 1, RMI_PAGE_L3,
                                                           RMI_OP_MEM_DELEGATED)) != RMI_SUCCESS)
        return VAL_ERROR;

    args->base = IPA_DATA;
    args->top = IPA_DATA + PAGE_SIZE;
    return VAL_SUCCESS;
}

static uint64_t check_rtte_state(void)
{
    val_host_pdev_ts pdev = {0};
    val_host_vdev_ts vdev = {0};
    val_host_realm_ts dev_realm = {0};
    val_host_pdev_flags_ts pdev_flags = {0};
    val_host_rtt_entry_ts rtte;
    uint32_t num_bdf, rp_bdf;
    uint64_t device_pa = 0;
    uint64_t ipa, ret = VAL_TEST_PREP_SEQ_FAILED;
    bool device_delegated = false;

    if (!val_host_rmm_supports_da())
        return VAL_SKIP_CHECK;

    num_bdf = val_pcie_get_num_bdf();
    if (num_bdf == VAL_ERROR ||
        val_pcie_find_doe_capability(&num_bdf, (uint32_t *)&pdev.bdf,
                                     &pdev.doe_cap_base) ||
        val_pcie_get_rootport((uint32_t)pdev.bdf, &rp_bdf))
        return VAL_TEST_PREP_SEQ_FAILED;

    pdev.root_id = (uint16_t)rp_bdf;
    val_host_set_pdev_flags(&pdev_flags);
    val_memcpy(&pdev.pdev_flags, &pdev_flags, sizeof(pdev_flags));
    vdev.vmid = VALID_REALM + 1U;

    vdev.vdev = g_vdev_state_prep_sequence(&pdev, &vdev, &dev_realm,
                                            RMI_VDEV_STARTED);
    if (vdev.vdev == VAL_TEST_PREP_SEQ_FAILED)
        goto cleanup;

    ipa = ipa_protected_unassigned_empty_prep_sequence(dev_realm.rd);
    if (ipa == VAL_TEST_PREP_SEQ_FAILED || pdev.ncoh_num_addr_range == 0U ||
        pdev.ncoh_addr_range[0].top - pdev.ncoh_addr_range[0].base < PAGE_SIZE)
        goto cleanup;

    device_pa = pdev.ncoh_addr_range[0].base;
    if (val_host_rmi_granule_range_delegate(device_pa,
                                             device_pa + PAGE_SIZE) != RMI_SUCCESS)
        goto cleanup;
    device_delegated = true;

    if (val_host_rmi_rtt_dev_map(dev_realm.rd, vdev.vdev, ipa,
                                  ipa + PAGE_SIZE, valid.flags,
                                  val_host_rmi_addr_range_desc(device_pa, 1, RMI_PAGE_L3,
                                                           RMI_OP_MEM_DELEGATED)) != RMI_SUCCESS)
        goto cleanup;

    dev_realm.dev_granules[0].ipa = ipa;
    dev_realm.dev_granules[0].size = PAGE_SIZE;
    dev_realm.dev_granules[0].level = MAP_LEVEL;
    dev_realm.dev_granules[0].pa = device_pa;
    dev_realm.dev_granules_mapped_count = 1;

    if (val_host_rmi_rtt_read_entry(dev_realm.rd, ipa, MAP_LEVEL, &rtte) !=
        RMI_SUCCESS || rtte.walk_level != MAP_LEVEL ||
        rtte.state != RMI_RTTE_NARCH_DEV)
        goto cleanup;

    ret = val_host_rmi_rtt_data_map(dev_realm.rd, ipa, ipa + PAGE_SIZE,
                                     valid.flags, valid.oaddr);

cleanup:
    if (vdev.vdev && vdev.vdev != VAL_TEST_PREP_SEQ_FAILED &&
        val_host_vdev_teardown(&dev_realm, &pdev, &vdev))
        ret = VAL_TEST_PREP_SEQ_FAILED;
    if (device_delegated &&
        val_host_rmi_granule_range_undelegate(device_pa,
                                               device_pa + PAGE_SIZE) != RMI_SUCCESS)
        ret = VAL_TEST_PREP_SEQ_FAILED;
    if (dev_realm.rd && val_host_realm_destroy(dev_realm.rd))
        ret = VAL_TEST_PREP_SEQ_FAILED;
    if (pdev.pdev && val_host_pdev_teardown(&pdev, pdev.pdev))
        ret = VAL_TEST_PREP_SEQ_FAILED;
    return ret;
}

static uint64_t intent_to_args(enum test_intent label, struct arguments *args)
{
    args->rd = valid.rd;
    args->base = valid.base;
    args->top = valid.top;
    args->flags = valid.flags;
    args->oaddr = valid.oaddr;

    switch (label) {
    case RD_UNALIGNED:
        args->rd++;
        break;
    case RD_STATE_UNDELEGATED:
        args->rd = g_undelegated_prep_sequence();
        break;
    case RD_STATE_DELEGATED:
        args->rd = g_delegated_prep_sequence();
        break;
    case RD_STATE_RTT:
        args->rd = valid.rtt;
        break;
    case RD_STATE_DATA:
        args->rd = g_data_prep_sequence(valid.rd, 6UL * PAGE_SIZE);
        break;
    case BASE_UNALIGNED:
        args->base++;
        break;
    case TOP_UNALIGNED:
        args->top++;
        break;
    case SIZE_INVALID:
        args->top = args->base;
        break;
    case IPA_UNPROTECTED:
        args->base = ipa_unprotected_unmapped_prep_sequence();
        args->top = args->base + PAGE_SIZE;
        break;
    case OADDR_SINGLE_UNALIGNED:
        /* Set the lowest address-field bit below RMI Granule alignment. */
        args->oaddr |= PAGE_SIZE >> 2;
        break;
    case OADDR_LIST_UNALIGNED:
        args->flags = (args->flags & ~0x3UL) | RMI_ADDR_TYPE_LIST;
        args->oaddr = 1;
        break;
    case OADDR_TYPE_NONE:
        args->flags = (args->flags & ~0x3UL) | RMI_ADDR_TYPE_NONE;
        break;
    case OADDR_TYPE_RESERVED:
        args->flags = (args->flags & ~0x3UL) | 3UL;
        break;
    case OADDR_LIST_INVALID_PAS:
        args->flags = (args->flags & ~0x3UL) | RMI_ADDR_TYPE_LIST;
        args->oaddr = g_outside_of_permitted_pa_prep_sequence();
        break;
    case OADDR_VALUE:
        return prepare_oaddr_value(args);
    case RTTE_SIZE:
        args->base = ipa_protected_unmapped_prep_sequence();
        args->top = args->base + PAGE_SIZE;
        break;
    case TRACKING_COARSE:
    {
        uint64_t ret = tracking_prep_sequence();

        if (ret != VAL_SUCCESS)
            return ret;
        args->oaddr = val_host_rmi_addr_range_desc(
            PLATFORM_TRACKING_TEST_REGION_BASE, 1, RMI_PAGE_L3,
            RMI_OP_MEM_DELEGATED);
        break;
    }
    case GRAN_STATE:
        args->oaddr = g_undelegated_prep_sequence();
        if (args->oaddr == VAL_TEST_PREP_SEQ_FAILED)
            return VAL_TEST_PREP_SEQ_FAILED;
        args->oaddr = val_host_rmi_addr_range_desc(args->oaddr, 1, RMI_PAGE_L3,
                                               RMI_OP_MEM_DELEGATED);
        break;
    default:
        return VAL_ERROR;
    }

    if (args->rd == VAL_TEST_PREP_SEQ_FAILED ||
        args->base == VAL_TEST_PREP_SEQ_FAILED ||
        args->oaddr == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_ERROR;
    return VAL_SUCCESS;
}

void cmd_rtt_data_map_host(void)
{
    struct arguments args;
    val_host_rtt_entry_ts rtte;
    val_smc_param_ts cmd;
    uint64_t ret;
    uint64_t i;

    if (prepare_valid_args() != VAL_SUCCESS) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        return;
    }

    for (i = 0; i < sizeof(test_data) / sizeof(test_data[0]); ++i) {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x\n",
            i + 1, test_data[i].msg, test_data[i].label);
        if (test_data[i].label == RTTE_STATE) {
            ret = check_rtte_state();
            if (ret == VAL_SKIP_CHECK) {
                LOG(ALWAYS, "Skipping rtte_state: DA is not supported\n");
                continue;
            }
        } else {
            ret = intent_to_args(test_data[i].label, &args);
            if (ret == VAL_SKIP_CHECK)
            {
                LOG(TEST, "Skipping Check %d\n", (int)(i + 1));
                continue;
            }
            if (ret != VAL_SUCCESS) {
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
                goto exit;
            }

            ret = val_host_rmi_rtt_data_map(args.rd, args.base, args.top,
                                            args.flags, args.oaddr);
        }
        if (test_data[i].label == TRACKING_COARSE)
        {
            if ((RMI_STATUS(ret) != RMI_ERROR_TRACKING) ||
                (RMI_TRACKING_LEVEL(ret) != RMI_TRACKING_LEVEL_COARSE) ||
                (VAL_EXTRACT_BITS(ret, 16, 55) !=
                 (PLATFORM_TRACKING_TEST_REGION_BASE >> PAGE_BITS_4K)))
            {
                LOG(ERROR, "DATA_MAP tracking check returned 0x%lx, expected "
                    "TRACKING at 0x%lx\n", ret, PLATFORM_TRACKING_TEST_REGION_BASE);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
                goto exit;
            }

            if (tracking_cleanup() != VAL_SUCCESS)
            {
                LOG(ERROR, "DATA_MAP tracking setup cleanup failed\n");
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
                goto exit;
            }
            continue;
        }

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "RMI_RTT_DATA_MAP returned 0x%x, expected 0x%x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    cmd = val_host_rmi_rtt_data_map_smc(valid.rd, valid.base, valid.top,
                                        valid.flags, valid.oaddr);
    ret = val_host_sro_complete(&cmd);
    if (ret != RMI_SUCCESS || cmd.x1 != valid.top ||
        val_host_rmi_rtt_read_entry(valid.rd, valid.base, MAP_LEVEL, &rtte) != 0 ||
        rtte.state != RMI_ASSIGNED ||
        OA(rtte.desc) != ADDR_ALIGN_DOWN(valid.oaddr, PAGE_SIZE)) {
        LOG(ERROR, "RMI_RTT_DATA_MAP positive check failed: 0x%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    val_host_update_granule_state(valid.rd, GRANULE_DATA,
                                  valid.data,
                                  valid.base, 0, 0);

    if (check_mapped_granule_state(valid.data) ||
        unmap_and_check_wiped(valid.base, valid.data)) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    if (check_list_positive()) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    if (tracking_cleanup() != VAL_SUCCESS)
    {
        LOG(ERROR, "DATA_MAP tracking setup cleanup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
    }
    return;
}
