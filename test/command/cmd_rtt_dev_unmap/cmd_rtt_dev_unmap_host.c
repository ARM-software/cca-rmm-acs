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
#include "rmi_rtt_dev_unmap_data.h"
#include "command_common_host.h"
#include "val_host_pcie.h"

static uint32_t num_bdf;

static struct argument_store {
    uint64_t rd_valid;
    uint64_t vdev_ptr_valid;
    uint64_t ipa_valid;
    uint64_t level_valid;
    uint64_t addr_valid;
    val_host_vdev_ts vdev_dev_valid;
    val_host_pdev_ts pdev_dev_valid;
    val_host_realm_ts realm_valid;
} c_args;

struct arguments {
    uint64_t rd;
    uint64_t base;
    uint64_t top;
    uint64_t flags;
    uint64_t oaddr;
};

static uint64_t output_list[PAGE_SIZE / sizeof(uint64_t)]
                            __attribute__((aligned(PAGE_SIZE)));

static uint64_t addr_range_desc_4k(uint64_t addr, uint64_t count)
{
    return val_host_rmi_addr_range_desc(addr, count, RMI_PAGE_L3,
                                    RMI_OP_MEM_DELEGATED);
}

static uint64_t rtt_dev_unmap_flags(uint8_t oaddr_type, uint16_t list_count)
{
    return ((uint64_t)oaddr_type & 0x3UL) |
           (((uint64_t)list_count & 0x3FFFUL) << 2);
}

static uint64_t valid_input_args_prep_sequence(void)
{
    val_host_pdev_ts pdev_dev;
    val_host_vdev_ts vdev_dev;
    val_host_realm_ts realm;
    val_host_pdev_flags_ts pdev_flags;
    uint32_t rp_bdf, status;
    uint64_t ret;
    uint64_t flags;
    uint64_t map_idx;
    val_host_rmi_rtt_prot_map_flags_ts map_flags;

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
                                                       RMI_VDEV_UNLOCKED);
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
        ((pdev_dev.ncoh_addr_range[0].top - pdev_dev.ncoh_addr_range[0].base) < PAGE_SIZE))
    {
        LOG(ERROR, "PDEV has no usable non-coherent device-memory range\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    c_args.addr_valid = pdev_dev.ncoh_addr_range[0].base;
    ret = val_host_rmi_granule_range_delegate(c_args.addr_valid,
                                               c_args.addr_valid + PAGE_SIZE);
    if (ret)
    {
        LOG(ERROR, "\tGranule delegation failed, PA=0x%lx ret=0x%lx\n",
            c_args.addr_valid, ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    val_memset(&map_flags, 0, sizeof(map_flags));
    map_flags.oaddr_type = RMI_ADDR_TYPE_SINGLE;
    val_memcpy(&flags, &map_flags, sizeof(map_flags));

    ret = val_host_rmi_rtt_dev_map(c_args.rd_valid, c_args.vdev_ptr_valid,
                                   c_args.ipa_valid, c_args.ipa_valid + PAGE_SIZE,
                                   flags, addr_range_desc_4k(c_args.addr_valid, 1U));
    if (ret)
    {
        LOG(ERROR, "\tRTT DEV map failed, ret=0x%lx\n", ret);
        (void)val_host_rmi_granule_range_undelegate(c_args.addr_valid,
                                                     c_args.addr_valid + PAGE_SIZE);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    map_idx = c_args.realm_valid.dev_granules_mapped_count;
    c_args.realm_valid.dev_granules[map_idx].ipa = c_args.ipa_valid;
    c_args.realm_valid.dev_granules[map_idx].size = PAGE_SIZE;
    c_args.realm_valid.dev_granules[map_idx].level = c_args.level_valid;
    c_args.realm_valid.dev_granules[map_idx].pa = c_args.addr_valid;
    c_args.realm_valid.dev_granules_mapped_count++;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    args->flags = rtt_dev_unmap_flags(RMI_ADDR_TYPE_SINGLE, 0U);
    args->oaddr = 0U;

    switch (label)
    {
        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            args->base = c_args.ipa_valid;
            args->top = c_args.ipa_valid + PAGE_SIZE;
            break;


        case RD_GRAN_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            args->base = c_args.ipa_valid;
            args->top = c_args.ipa_valid + PAGE_SIZE;
            break;

        case BASE_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->base = g_unaligned_prep_sequence(c_args.ipa_valid);
            args->top = c_args.ipa_valid + PAGE_SIZE;
            break;

        case TOP_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->base = c_args.ipa_valid;
            args->top = g_unaligned_prep_sequence(c_args.ipa_valid + PAGE_SIZE);
            break;

        case SIZE_INVALID:
            args->rd = c_args.rd_valid;
            args->base = c_args.ipa_valid;
            args->top = c_args.ipa_valid;
            break;

        case IPA_UNPROTECTED:
            args->rd = c_args.rd_valid;
            args->base = ipa_unprotected_unmapped_prep_sequence();
            args->top = args->base + PAGE_SIZE;
            break;

        case RTT_WALK:
            args->rd = c_args.rd_valid;
            args->base = ipa_protected_unmapped_prep_sequence();
            args->top = args->base + PAGE_SIZE;
            break;

        case RTTE_STATE:
            args->rd = c_args.rd_valid;
            args->base = ipa_protected_assigned_empty_prep_sequence(c_args.rd_valid);
            if (args->base == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_TEST_PREP_SEQ_FAILED;
            args->top = args->base + PAGE_SIZE;
            break;

        case OADDR_LIST_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->base = c_args.ipa_valid;
            args->top = c_args.ipa_valid + PAGE_SIZE;
            args->flags = rtt_dev_unmap_flags(RMI_ADDR_TYPE_LIST, 1U);
            args->oaddr = (uint64_t)output_list + 4U;
            break;

        case OADDR_LIST_INVALID_PAS:
            args->rd = c_args.rd_valid;
            args->base = c_args.ipa_valid;
            args->top = c_args.ipa_valid + PAGE_SIZE;
            args->flags = rtt_dev_unmap_flags(RMI_ADDR_TYPE_LIST, 1U);
            args->oaddr = g_secure_prep_sequence();
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_rtt_dev_unmap_host(void)
{
    uint64_t i;
    uint64_t ret, flags;
    val_smc_param_ts cmd_ret;
    struct arguments args;
    val_host_rtt_entry_ts rtte;
    val_host_rmi_rtt_unmap_flags_ts unmap_flags;

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

    val_memset(&unmap_flags, 0, sizeof(unmap_flags));
    unmap_flags.oaddr_type = RMI_ADDR_TYPE_SINGLE;
    val_memcpy(&flags, &unmap_flags, sizeof(unmap_flags));

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "\n\tCheck %d : ", i + 1);
        LOG(TEST, test_data[i].msg);
        LOG(TEST, "; intent id : 0x%x \n", test_data[i].label);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto destroy_device;
        }

        cmd_ret = val_host_rmi_rtt_dev_unmap_smc(args.rd, args.base, args.top,
                                                 args.flags, args.oaddr);
        ret = val_host_sro_complete(&cmd_ret);

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_device;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n");
    cmd_ret = val_host_rmi_rtt_dev_unmap_smc(c_args.rd_valid, c_args.ipa_valid,
                                             c_args.ipa_valid + PAGE_SIZE,
                                             flags, 0U);
    ret = val_host_sro_complete(&cmd_ret);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "Command failed. %x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }
    c_args.realm_valid.dev_granules_mapped_count = 0;

    if (cmd_ret.x1 != (c_args.ipa_valid + PAGE_SIZE))
    {
        LOG(ERROR, "\tUnexpected top IPA: 0x%lx\n", cmd_ret.x1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_device;
    }

    if (cmd_ret.x2 != addr_range_desc_4k(c_args.addr_valid, 1U))
    {
        LOG(ERROR, "\tUnexpected output range descriptor: 0x%lx\n", cmd_ret.x2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_device;
    }

    if (cmd_ret.x3 != 0U)
    {
        LOG(ERROR, "\tUnexpected output list count: 0x%lx\n", cmd_ret.x3);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_device;
    }

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                      c_args.level_valid, &rtte);
    if (ret)
    {
        LOG(ERROR, "rtt_read_entry failed ret = %lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_device;
    }

    if ((rtte.state != RMI_UNASSIGNED) || (rtte.ripas != RMI_EMPTY))
    {
        LOG(ERROR, "Unexpected RTTE state after unmap: hipas %lu ripas %lu\n",
            rtte.state, rtte.ripas);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_device;
    }

    /* Map the same delegated page again to exercise LIST output. */
    ret = val_host_rmi_rtt_dev_map(c_args.rd_valid, c_args.vdev_ptr_valid,
                                   c_args.ipa_valid, c_args.ipa_valid + PAGE_SIZE,
                                   RMI_ADDR_TYPE_SINGLE,
                                   addr_range_desc_4k(c_args.addr_valid, 1U));
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "LIST setup map failed, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
        goto destroy_device;
    }
    c_args.realm_valid.dev_granules[0].ipa = c_args.ipa_valid;
    c_args.realm_valid.dev_granules[0].size = PAGE_SIZE;
    c_args.realm_valid.dev_granules[0].level = c_args.level_valid;
    c_args.realm_valid.dev_granules[0].pa = c_args.addr_valid;
    c_args.realm_valid.dev_granules_mapped_count = 1;

    val_memset(output_list, 0, sizeof(output_list));
    cmd_ret = val_host_rmi_rtt_dev_unmap_smc(c_args.rd_valid, c_args.ipa_valid,
                                              c_args.ipa_valid + PAGE_SIZE,
                                              rtt_dev_unmap_flags(RMI_ADDR_TYPE_LIST, 1U),
                                              (uint64_t)output_list);
    ret = val_host_sro_complete(&cmd_ret);
    if (ret == RMI_SUCCESS)
        c_args.realm_valid.dev_granules_mapped_count = 0;
    if (ret != RMI_SUCCESS || cmd_ret.x1 != c_args.ipa_valid + PAGE_SIZE ||
        cmd_ret.x2 != 0U || cmd_ret.x3 != 1U ||
        output_list[0] != addr_range_desc_4k(c_args.addr_valid, 1U))
    {
        LOG(ERROR, "LIST unmap output mismatch, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
        goto destroy_device;
    }
    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                      c_args.level_valid, &rtte);
    if (ret != RMI_SUCCESS || rtte.state != RMI_UNASSIGNED)
    {
        LOG(ERROR, "LIST unmap did not restore VOID RTTE, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(14)));
        goto destroy_device;
    }

    /* NONE unmaps the page without returning its physical address. */
    ret = val_host_rmi_rtt_dev_map(c_args.rd_valid, c_args.vdev_ptr_valid,
                                   c_args.ipa_valid, c_args.ipa_valid + PAGE_SIZE,
                                   RMI_ADDR_TYPE_SINGLE,
                                   addr_range_desc_4k(c_args.addr_valid, 1U));
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "NONE setup map failed, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(15)));
        goto destroy_device;
    }
    c_args.realm_valid.dev_granules_mapped_count = 1;

    cmd_ret = val_host_rmi_rtt_dev_unmap_smc(c_args.rd_valid, c_args.ipa_valid,
                                              c_args.ipa_valid + PAGE_SIZE,
                                              rtt_dev_unmap_flags(RMI_ADDR_TYPE_NONE, 0U),
                                              0U);
    ret = val_host_sro_complete(&cmd_ret);
    if (ret == RMI_SUCCESS)
        c_args.realm_valid.dev_granules_mapped_count = 0;
    if (ret != RMI_SUCCESS || cmd_ret.x1 != c_args.ipa_valid + PAGE_SIZE ||
        cmd_ret.x2 != 0U || cmd_ret.x3 != 0U)
    {
        LOG(ERROR, "NONE unmap output mismatch, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(16)));
        goto destroy_device;
    }

    ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, c_args.ipa_valid,
                                      c_args.level_valid, &rtte);
    if (ret != RMI_SUCCESS || rtte.state != RMI_UNASSIGNED)
    {
        LOG(ERROR, "NONE unmap did not restore VOID RTTE, ret=0x%lx\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(17)));
        goto destroy_device;
    }

    ret = val_host_rmi_granule_range_undelegate(c_args.addr_valid,
                                                 c_args.addr_valid + PAGE_SIZE);
    if (ret)
    {
        LOG(ERROR, "Granule undelegation failed, PA=0x%lx ret=0x%lx\n",
            c_args.addr_valid, ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_device;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    if (c_args.vdev_ptr_valid &&
        val_host_vdev_teardown(&c_args.realm_valid, &c_args.pdev_dev_valid,
                               &c_args.vdev_dev_valid))
    {
        LOG(ERROR, "VDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto exit;
    }

    if (c_args.pdev_dev_valid.pdev &&
        val_host_pdev_teardown(&c_args.pdev_dev_valid, c_args.pdev_dev_valid.pdev))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto exit;
    }

exit:
    return;
}
