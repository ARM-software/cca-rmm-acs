/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_vdev_map_data.h"
#include "command_common_host.h"
#include "val_host_pcie.h"

#define VALID_REALM   1
#define VDEV_MAP_TEST_IPA   0x4000100000

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
    uint64_t vdev_ptr;
    uint64_t ipa;
    uint64_t level;
    uint64_t addr;
};

static uint64_t rd_new_prep_sequence(void)
{
    val_host_realm_ts realm;

    val_memset(&realm, 0, sizeof(realm));

    realm.s2sz = 40;
    realm.hash_algo = RMI_HASH_SHA_256;
    realm.s2_starting_level = 0;
    realm.num_s2_sl_rtts = 1;
    realm.vmid = VALID_REALM;

    if (val_host_realm_create_common(&realm))
    {
        LOG(ERROR, "\tRealm create failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return realm.rd;
}

static uint64_t level_oob_prep_sequence(void)
{
    return VAL_RTT_MAX_LEVEL + 1;
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
    uint64_t ret;

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

    c_args.addr_valid = VDEV_MAP_TEST_IPA;

    ret = val_host_rmi_granule_delegate(c_args.addr_valid);
    if (ret)
    {
        LOG(ERROR, "\tGranule delegation failed, PA=0x%lx ret=0x%lx\n",
            c_args.addr_valid, ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case ADDR_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->addr = g_unaligned_prep_sequence(c_args.addr_valid);
            break;

        case ADDR_OUTSIDE_OF_PERMITTED_PA:
            args->rd = c_args.rd_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->addr = g_outside_of_permitted_pa_prep_sequence();
            break;

        case ADDR_DEV_MEM_MMIO:
            args->rd = c_args.rd_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->addr = g_dev_mem_prep_sequence();
            break;

        case ADDR_GRAN_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->addr = g_undelegated_prep_sequence();
            break;

        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->addr = c_args.addr_valid;
            break;

        case RD_OUTSIDE_OF_PERMITTED_PA:
            args->rd = g_outside_of_permitted_pa_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->addr = c_args.addr_valid;
            break;

        case RD_DEV_MEM_MMIO:
            args->rd = g_dev_mem_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->addr = c_args.addr_valid;
            break;

        case RD_GRAN_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->addr = c_args.addr_valid;
            break;

        case VDEV_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->vdev_ptr = g_unaligned_prep_sequence(c_args.vdev_ptr_valid);
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->addr = c_args.addr_valid;
            break;

        case VDEV_OUTSIDE_OF_PERMITTED_PA:
            args->rd = c_args.rd_valid;
            args->vdev_ptr = g_outside_of_permitted_pa_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->addr = c_args.addr_valid;
            break;

        case VDEV_DEV_MEM_MMIO:
            args->rd = c_args.rd_valid;
            args->vdev_ptr = g_dev_mem_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->addr = c_args.addr_valid;
            break;

        case VDEV_GRAN_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->vdev_ptr = g_undelegated_prep_sequence();
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->addr = c_args.addr_valid;
            break;

        case INVALID_RD:
            args->rd = rd_new_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_TEST_PREP_SEQ_FAILED;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->ipa = c_args.ipa_valid;
            args->level = c_args.level_valid;
            args->addr = c_args.addr_valid;
            break;

        case LEVEL_LT_2:
            args->rd = c_args.rd_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->ipa = c_args.ipa_valid;
            args->level = 1;
            args->addr = c_args.addr_valid;
            break;

        case LEVEL_OOB:
            args->rd = c_args.rd_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->ipa = c_args.ipa_valid;
            args->level = level_oob_prep_sequence();
            args->addr = c_args.addr_valid;
            break;

        case IPA_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->ipa = g_unaligned_prep_sequence(c_args.ipa_valid);
            args->level = c_args.level_valid;
            args->addr = c_args.addr_valid;
            break;

        case IPA_UNPROTECTED:
            args->rd = c_args.rd_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->ipa = ipa_unprotected_unassigned_prep_sequence(c_args.rd_valid);
            args->level = c_args.level_valid;
            args->addr = c_args.addr_valid;
            break;

        case RTT_WALK:
            args->rd = c_args.rd_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->ipa = ipa_protected_unmapped_prep_sequence();
            args->level = c_args.level_valid;
            args->addr = c_args.addr_valid;
            break;

        case RTTE_STATE:
            args->rd = c_args.rd_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->ipa = ipa_protected_assigned_empty_prep_sequence(c_args.rd_valid);
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_TEST_PREP_SEQ_FAILED;
            args->level = c_args.level_valid;
            args->addr = c_args.addr_valid;
            break;

        case RTTE_RIPAS:
            args->rd = c_args.rd_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->ipa = ipa_unassigned_ram_prep_sequence();
            if (args->ipa == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_TEST_PREP_SEQ_FAILED;
            args->level = c_args.level_valid;
            args->addr = c_args.addr_valid;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_vdev_map_host(void)
{
    uint64_t i;
    uint64_t ret;
    val_smc_param_ts cmd_ret;
    struct arguments args;
    val_host_rtt_entry_ts rtte;
    uint64_t map_idx;

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

        cmd_ret = val_host_rmi_vdev_map(args.rd, args.vdev_ptr, args.ipa, args.level, args.addr);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_device;
        }
    }

    LOG(TEST, "\n\tPositive Observability Check\n");
    cmd_ret = val_host_rmi_vdev_map(c_args.rd_valid, c_args.vdev_ptr_valid, c_args.ipa_valid,
                                    c_args.level_valid, c_args.addr_valid);
    if (cmd_ret.x0 != 0)
    {
        LOG(ERROR, "Command failed. %x\n", cmd_ret.x0);
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

    if ((rtte.state != RMI_ASSIGNED_DEV))
    {
        LOG(ERROR, "Unexpected RTTE state: hipas %lu\n", rtte.state);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_device;
    }

    map_idx = c_args.realm_valid.dev_granules_mapped_count;
    c_args.realm_valid.dev_granules[map_idx].ipa = c_args.ipa_valid;
    c_args.realm_valid.dev_granules[map_idx].size = PAGE_SIZE;
    c_args.realm_valid.dev_granules[map_idx].level = c_args.level_valid;
    c_args.realm_valid.dev_granules[map_idx].pa = c_args.addr_valid;
    c_args.realm_valid.dev_granules_mapped_count++;

    val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    if (c_args.vdev_ptr_valid &&
        val_host_vdev_teardown(&c_args.realm_valid, &c_args.pdev_dev_valid,
                               &c_args.vdev_dev_valid))
    {
        LOG(ERROR, "VDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

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
