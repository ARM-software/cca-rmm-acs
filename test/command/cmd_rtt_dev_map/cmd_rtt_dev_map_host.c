/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_rmi_sro.h"
#include "val_host_sro.h"
#include "rmi_rtt_dev_map_data.h"
#include "command_common_host.h"
#include "val_host_pcie.h"

#define INVALID_REALM_VMID  2

static uint32_t num_bdf;

static struct argument_store {
    uint64_t rd_valid;
    uint64_t vdev_ptr_valid;
    uint64_t ipa_valid;
    uint64_t level_valid;
    uint64_t addr_valid;
    uint64_t addr_alt_valid;
    uint64_t addr_bound;
    uint64_t addr_list;
    val_host_vdev_ts vdev_dev_valid;
    val_host_pdev_ts pdev_dev_valid;
    val_host_realm_ts realm_valid;
} c_args;

static struct invalid_argument_store {
    val_host_realm_ts realm_invalid;
} c_args_invalid;

struct arguments {
    uint64_t rd;
    uint64_t vdev_ptr;
    uint64_t base;
    uint64_t top;
    uint64_t flags;
    uint64_t oaddr;
    bool cleanup_mapping;
};

static uint64_t addr_range_desc_4k(uint64_t addr, uint64_t count)
{
    return val_host_rmi_addr_range_desc(addr, count, RMI_PAGE_L3,
                                    RMI_OP_MEM_DELEGATED);
}

static uint64_t rtt_dev_map_flags(uint8_t oaddr_type, uint16_t list_count)
{
    return ((uint64_t)oaddr_type & 0x3UL) |
           (((uint64_t)list_count & 0x3FFFUL) << 2U);
}

static uint64_t rd_new_prep_sequence(void)
{
    val_host_realm_ts *realm = &c_args_invalid.realm_invalid;

    val_memset(realm, 0, sizeof(*realm));

    realm->s2sz = 40;
    realm->hash_algo = RMI_HASH_SHA_256;
    realm->s2_starting_level = 0;
    realm->num_s2_sl_rtts = 1;
    realm->vmid = INVALID_REALM_VMID;

    if (val_host_realm_create_common(realm))
    {
        LOG(ERROR, "\tRealm create failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm->rd;
}

static uint64_t ipa_unassigned_ram_prep_sequence(void)
{
    uint64_t ret = 0;
    val_host_rec_enter_ts *rec_enter = NULL;
    val_host_rec_exit_ts *rec_exit = NULL;
    uint64_t out_top;
    val_host_rtt_entry_ts rtte;
    uint64_t ripas_ipa = 0x800000, ripas_size = 0x1000;

    if (create_mapping(ripas_ipa, false, c_args.rd_valid))
    {
        LOG(ERROR, "Protected Unassinged ipa mapping creation failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    rec_enter = &(((val_host_rec_run_ts *)c_args.realm_valid.run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)c_args.realm_valid.run[0])->exit);

    /* REC enter REC[0] execution */
    ret = val_host_rmi_rec_enter(c_args.realm_valid.rec[0], c_args.realm_valid.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        goto exit;
    }
    else if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)c_args.realm_valid.run[0]))
    {
        LOG(ERROR, "REC_EXIT: HOST_CALL params mismatch\n");
        goto exit;
    }

    rec_enter->gprs[1] = ripas_ipa;
    rec_enter->gprs[2] = ripas_size;

    ret = val_host_rmi_rec_enter(c_args.realm_valid.rec[0], c_args.realm_valid.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        goto exit;
    }
    else if (val_host_check_realm_exit_ripas_change(
        (val_host_rec_run_ts *)c_args.realm_valid.run[0]))
    {
        LOG(ERROR, "Ripas change req failed, %x\n", rec_exit->exit_reason);
        goto exit;
    }

    if (!(rec_exit->ripas_base == ripas_ipa) || !(rec_exit->ripas_top == (ripas_ipa + ripas_size)))
    {
        LOG(ERROR, "Ripas return params mismatch\n");
        goto exit;
    }

    ret = val_host_rmi_rtt_set_ripas(c_args.realm_valid.rd, c_args.realm_valid.rec[0],
                                 rec_exit->ripas_base, rec_exit->ripas_top, &out_top);
    if (ret)
    {
        LOG(ERROR, "RMI_RTT_SET_RIPAS failed, ret=%x\n", ret);
        goto exit;
    }

    ret = val_host_rmi_rtt_read_entry(c_args.realm_valid.rd, ripas_ipa, VAL_RTT_MAX_LEVEL, &rtte);
    if (ret)
    {
        LOG(ERROR, "rtt_read_entry failed ret = %x\n", ret);
        goto exit;
    }

    if ((rtte.state != RMI_UNASSIGNED) || (rtte.ripas != RMI_RAM))
    {
        LOG(ERROR, "Unexpected RTTE states: hipas %d ripas %d\n", rtte.state, rtte.ripas);
        goto exit;
    }

    /* rec enter for letting the realm know the service request is finished*/
    ret = val_host_rmi_rec_enter(c_args.realm_valid.rec[0], c_args.realm_valid.run[0]);
    if (ret)
    {
        LOG(ERROR, "Rec enter failed, ret=%x\n", ret);
        goto exit;
    }
    else if (val_host_check_realm_exit_host_call((val_host_rec_run_ts *)c_args.realm_valid.run[0]))
    {
        LOG(ERROR, "REC_EXIT: HOST_CALL params mismatch\n");
        goto exit;
    }

    return ripas_ipa;

exit:
    return VAL_TEST_PREP_SEQ_FAILED;

}

static uint64_t valid_input_args_prep_sequence(void)
{
    val_host_pdev_ts pdev_dev;
    val_host_vdev_ts vdev_dev;
    val_host_realm_ts realm;
    val_host_pdev_flags_ts pdev_flags;
    uint32_t rp_bdf, status;
    uint64_t ret, i;

    val_memset(&pdev_dev, 0, sizeof(pdev_dev));
    val_memset(&vdev_dev, 0, sizeof(vdev_dev));
    val_memset(&realm, 0, sizeof(realm));
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

    c_args.vdev_ptr_valid = g_vdev_state_prep_sequence(&pdev_dev, &vdev_dev, &realm,
                                                       RMI_VDEV_STARTED);
    if (c_args.vdev_ptr_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.rd_valid = vdev_dev.realm_rd;
    c_args.vdev_dev_valid = vdev_dev;
    c_args.pdev_dev_valid = pdev_dev;
    c_args.realm_valid = realm;
    c_args.level_valid = VAL_RTT_MAX_LEVEL;

    c_args.ipa_valid = ipa_protected_unassigned_empty_prep_sequence(c_args.rd_valid);
    if (c_args.ipa_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    if ((pdev_dev.ncoh_num_addr_range == 0U) ||
        ((pdev_dev.ncoh_addr_range[0].top - pdev_dev.ncoh_addr_range[0].base) <
         (2U * PAGE_SIZE)))
    {
        LOG(ERROR, "PDEV needs two pages of non-coherent device memory\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    c_args.addr_valid = pdev_dev.ncoh_addr_range[0].base;
    c_args.addr_alt_valid = c_args.addr_valid + PAGE_SIZE;

    ret = val_host_rmi_granule_range_delegate(c_args.addr_valid,
                                               c_args.addr_valid + (2U * PAGE_SIZE));
    if (ret)
    {
        LOG(ERROR, "\tGranule delegation failed, PA=0x%lx ret=0x%lx\n",
            c_args.addr_valid, ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    c_args.addr_list = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    c_args.addr_bound = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if ((c_args.addr_list == 0U) || (c_args.addr_bound == 0U))
    {
        LOG(ERROR, "Failed to allocate map-test address buffers\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    val_memset((void *)c_args.addr_list, 0, PAGE_SIZE);
    *(uint64_t *)c_args.addr_list = addr_range_desc_4k(c_args.addr_valid, 1U);

    for (i = 0; i < pdev_dev.ncoh_num_addr_range; i++)
    {
        if ((c_args.addr_bound >= pdev_dev.ncoh_addr_range[i].base) &&
            (c_args.addr_bound < pdev_dev.ncoh_addr_range[i].top))
        {
            LOG(ERROR, "Bound-test address unexpectedly belongs to the VDEV range\n");
            return VAL_TEST_PREP_SEQ_FAILED;
        }
    }

    ret = val_host_rmi_granule_range_delegate(c_args.addr_bound,
                                               c_args.addr_bound + PAGE_SIZE);
    if (ret)
    {
        LOG(ERROR, "Bound-test granule delegation failed, PA=0x%lx ret=0x%lx\n",
            c_args.addr_bound, ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return VAL_SUCCESS;
}

static uint64_t prepare_oaddr_value(struct arguments *args)
{
    uint64_t ret;

    ret = val_host_rmi_rtt_dev_map(c_args.rd_valid, c_args.vdev_ptr_valid,
                                   c_args.ipa_valid,
                                   c_args.ipa_valid + PAGE_SIZE,
                                   rtt_dev_map_flags(RMI_ADDR_TYPE_SINGLE, 0U),
                                   addr_range_desc_4k(c_args.addr_alt_valid, 1U));
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "Failed to prepare existing device mapping, ret=0x%lx\n", ret);
        return VAL_ERROR;
    }

    args->cleanup_mapping = true;
    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;
    uint64_t addr;

    args->rd = c_args.rd_valid;
    args->vdev_ptr = c_args.vdev_ptr_valid;
    args->base = c_args.ipa_valid;
    args->top = c_args.ipa_valid + PAGE_SIZE;
    args->flags = rtt_dev_map_flags(RMI_ADDR_TYPE_SINGLE, 0U);
    args->oaddr = addr_range_desc_4k(c_args.addr_valid, 1U);
    args->cleanup_mapping = false;

    switch (label)
    {
        case GRAN_STATE_UNDELEGATED:
            addr = g_undelegated_prep_sequence();
            if (addr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            args->oaddr = addr_range_desc_4k(addr, 1U);
            break;
        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            break;

        case RD_GRAN_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            break;
        case VDEV_UNALIGNED:
            args->vdev_ptr = g_unaligned_prep_sequence(c_args.vdev_ptr_valid);
            break;

        case VDEV_GRAN_STATE_UNDELEGATED:
            args->vdev_ptr = g_undelegated_prep_sequence();
            break;
        case VDEV_REALM:
            args->rd = rd_new_prep_sequence();
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
            args->base = ipa_unprotected_unassigned_prep_sequence(c_args.rd_valid);
            args->top = args->base + PAGE_SIZE;
            break;
        case OADDR_LIST_UNALIGNED:
            args->flags = rtt_dev_map_flags(RMI_ADDR_TYPE_LIST, 1U);
            args->oaddr = c_args.addr_list + 4U;
            break;
        case OADDR_TYPE_RESERVED:
            args->flags = rtt_dev_map_flags(3U, 0U);
            break;
        case OADDR_TYPE_NONE:
            args->flags = rtt_dev_map_flags(RMI_ADDR_TYPE_NONE, 0U);
            break;
        case OADDR_LIST_INVALID_PAS:
            args->flags = rtt_dev_map_flags(RMI_ADDR_TYPE_LIST, 1U);
            args->oaddr = c_args.addr_valid;
            break;
        case OADDR_VALUE:
            if (prepare_oaddr_value(args) != VAL_SUCCESS)
                return VAL_ERROR;
            break;
        case RTTE_SIZE:
            args->base = ipa_protected_unmapped_prep_sequence();
            args->top = args->base + PAGE_SIZE;
            break;
        case RTTE_STATE:
            args->base = ipa_protected_assigned_empty_prep_sequence(c_args.rd_valid);
            args->top = args->base + PAGE_SIZE;
            break;
        case RTTE_RIPAS:
            args->base = ipa_unassigned_ram_prep_sequence();
            args->top = args->base + PAGE_SIZE;
            break;
        case OADDR_BOUND:
            args->oaddr = addr_range_desc_4k(c_args.addr_bound, 1U);
            break;
        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    if ((args->rd == VAL_TEST_PREP_SEQ_FAILED) ||
        (args->base == VAL_TEST_PREP_SEQ_FAILED) ||
        (args->oaddr == VAL_TEST_PREP_SEQ_FAILED))
        return VAL_ERROR;

    return VAL_SUCCESS;
}

void cmd_rtt_dev_map_host(void)
{
    uint64_t i;
    uint64_t ret;
    struct arguments args;
    val_host_rtt_entry_ts rtte;
    val_smc_param_ts cmd;
    uint64_t map_idx;
    uint64_t list_ipa;

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

        ret = val_host_rmi_rtt_dev_map(args.rd, args.vdev_ptr,
                                       args.base, args.top,
                                       args.flags, args.oaddr);

        if (args.cleanup_mapping) {
            uint64_t cleanup_ret;

            cleanup_ret = val_host_rmi_rtt_dev_unmap(c_args.rd_valid,
                                                     c_args.ipa_valid,
                                                     c_args.ipa_valid + PAGE_SIZE,
                                                     RMI_ADDR_TYPE_SINGLE, 0U);
            if (cleanup_ret != RMI_SUCCESS) {
                LOG(ERROR, "Existing mapping cleanup failed, ret=0x%lx\n",
                    cleanup_ret);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
                goto destroy_device;
            }
        }

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));

            /* Keep teardown usable if an invalid mapping was unexpectedly accepted. */
            if ((ret == RMI_SUCCESS) && !args.cleanup_mapping) {
                uint64_t cleanup_ret;

                cleanup_ret = val_host_rmi_rtt_dev_unmap(args.rd, args.base,
                                                         args.top,
                                                         RMI_ADDR_TYPE_SINGLE, 0U);
                if (cleanup_ret != RMI_SUCCESS) {
                    LOG(ERROR, "Unexpected mapping cleanup failed, ret=%x\n",
                        cleanup_ret);
                }
            }

            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_device;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n");
    cmd = val_host_rmi_rtt_dev_map_smc(c_args.rd_valid, c_args.vdev_ptr_valid,
                                       c_args.ipa_valid, c_args.ipa_valid + PAGE_SIZE,
                                       rtt_dev_map_flags(RMI_ADDR_TYPE_SINGLE, 0U),
                                       addr_range_desc_4k(c_args.addr_valid, 1U));
    ret = val_host_sro_complete(&cmd);
    if (ret != 0)
    {
        LOG(ERROR, "Command failed. %x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    map_idx = c_args.realm_valid.dev_granules_mapped_count;
    c_args.realm_valid.dev_granules[map_idx].ipa = c_args.ipa_valid;
    c_args.realm_valid.dev_granules[map_idx].size = PAGE_SIZE;
    c_args.realm_valid.dev_granules[map_idx].level = c_args.level_valid;
    c_args.realm_valid.dev_granules[map_idx].pa = c_args.addr_valid;
    c_args.realm_valid.dev_granules_mapped_count++;

    if (cmd.x1 != c_args.ipa_valid + PAGE_SIZE) {
        LOG(ERROR, "Unexpected SINGLE mapping out_top: 0x%lx\n", cmd.x1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid, c_args.level_valid, &rtte);
    if (ret)
    {
        LOG(ERROR, "rtt_read_entry failed ret = %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_device;
    }

    if ((rtte.state != RMI_RTTE_NARCH_DEV) ||
        (VAL_EXTRACT_BITS(rtte.desc, 2, 9) != 0))
    {
        LOG(ERROR, "Unexpected NARCH_DEV state or attributes\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_device;
    }

    if (OA(rtte.desc) != c_args.addr_valid)
    {
        LOG(ERROR, "Unexpected RTTE PA: expected 0x%lx, returned 0x%lx\n",
            c_args.addr_valid, OA(rtte.desc));
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_device;
    }

    LOG(TEST, "\n\tLIST mapping success\n");
    /* The intervening IPA is assigned by the rtte_state failure check. */
    list_ipa = c_args.ipa_valid + (2U * PAGE_SIZE);
    if (create_mapping(list_ipa, false, c_args.rd_valid)) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_device;
    }
    *(uint64_t *)c_args.addr_list = addr_range_desc_4k(c_args.addr_alt_valid, 1U);
    cmd = val_host_rmi_rtt_dev_map_smc(c_args.rd_valid, c_args.vdev_ptr_valid,
                                       list_ipa, list_ipa + PAGE_SIZE,
                                       rtt_dev_map_flags(RMI_ADDR_TYPE_LIST, 1U),
                                       c_args.addr_list);
    ret = val_host_sro_complete(&cmd);
    if (ret == RMI_SUCCESS) {
        map_idx = c_args.realm_valid.dev_granules_mapped_count;
        c_args.realm_valid.dev_granules[map_idx].ipa = list_ipa;
        c_args.realm_valid.dev_granules[map_idx].size = PAGE_SIZE;
        c_args.realm_valid.dev_granules[map_idx].level = c_args.level_valid;
        c_args.realm_valid.dev_granules[map_idx].pa = c_args.addr_alt_valid;
        c_args.realm_valid.dev_granules_mapped_count++;
    }
    if (ret != RMI_SUCCESS || cmd.x1 != list_ipa + PAGE_SIZE) {
        LOG(ERROR, "LIST RMI_RTT_DEV_MAP failed, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_device;
    }

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, list_ipa,
                                      c_args.level_valid, &rtte);
    if (ret != RMI_SUCCESS || rtte.state != RMI_RTTE_NARCH_DEV ||
        OA(rtte.desc) != c_args.addr_alt_valid) {
        LOG(ERROR, "LIST mapping RTTE mismatch, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto destroy_device;
    }

    if (IS_TEST_START(val_get_status()))
        val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    if (c_args_invalid.realm_invalid.rd &&
        val_host_realm_destroy(c_args_invalid.realm_invalid.rd))
    {
        LOG(ERROR, "Invalid Realm teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
    }

    if (c_args.vdev_ptr_valid &&
        val_host_vdev_teardown(&c_args.realm_valid, &c_args.pdev_dev_valid,
                               &c_args.vdev_dev_valid))
    {
        LOG(ERROR, "VDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    if (c_args.addr_valid &&
        val_host_rmi_granule_range_undelegate(c_args.addr_valid,
                                               c_args.addr_valid + PAGE_SIZE))
    {
        LOG(ERROR, "Device-memory granule cleanup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    if (c_args.addr_alt_valid &&
        val_host_rmi_granule_range_undelegate(c_args.addr_alt_valid,
                                               c_args.addr_alt_valid + PAGE_SIZE))
    {
        LOG(ERROR, "Alternate device-memory granule cleanup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    if (c_args.addr_bound &&
        val_host_rmi_granule_range_undelegate(c_args.addr_bound,
                                               c_args.addr_bound + PAGE_SIZE))
    {
        LOG(ERROR, "Bound-test granule cleanup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto exit;
    }

    if (c_args.addr_list)
        val_host_mem_free((void *)c_args.addr_list);
    if (c_args.addr_bound)
        val_host_mem_free((void *)c_args.addr_bound);

    if (c_args.pdev_dev_valid.pdev &&
        val_host_pdev_teardown(&c_args.pdev_dev_valid, c_args.pdev_dev_valid.pdev))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

exit:
    return;
}
