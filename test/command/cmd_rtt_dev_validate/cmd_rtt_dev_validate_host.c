/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_rmi_sro.h"
#include "rmi_rtt_dev_validate_data.h"
#include "command_common_host.h"
#include "val_host_pcie.h"
#include "val_host_helpers.h"

#define INVALID_REALM_VMID 2

static uint32_t num_bdf;

static struct argument_store {
    uint64_t rd_valid;
    uint64_t rec_ptr_valid;
    uint64_t pdev_ptr_valid;
    uint64_t vdev_ptr_valid;
    uint64_t base_valid;
    uint64_t top_valid;
    uint64_t pa_valid;
    uint64_t report_size;
    val_host_vdev_ts vdev_dev_valid;
    val_host_pdev_ts pdev_dev_valid;
    val_host_realm_ts realm_valid;
} c_args;

static struct invalid_argument_store {
    val_host_realm_ts realm_invalid;
} c_args_invalid;

struct arguments {
    uint64_t rd;
    uint64_t rec_ptr;
    uint64_t base;
    uint64_t top;
};

/*
 * Verify all REC exit fields are zero except those explicitly allowed
 * (VDEV mapping exits, gicv3_*, cnt*, and pmu_ovf_status).
 */
static uint32_t validate_rec_exit_zero_fields(val_host_rec_exit_ts *rec_exit)
{
    val_host_rec_exit_ts exit_copy;
    val_host_rec_exit_ts expected_zero_exit;
    uint8_t *exit_bytes;
    size_t i;
    uint64_t value;

    val_memcpy(&exit_copy, rec_exit, sizeof(exit_copy));
    val_memset(&expected_zero_exit, 0, sizeof(expected_zero_exit));

    exit_copy.exit_reason = 0;
    exit_copy.dev_mem_base = 0;
    exit_copy.dev_mem_top = 0;
    exit_copy.dev_mem_pa = 0;
    exit_copy.vdev_id_1 = 0;

    exit_copy.gicv3_hcr = 0;
    val_memset(exit_copy.gicv3_lrs, 0, sizeof(exit_copy.gicv3_lrs));
    exit_copy.gicv3_misr = 0;
    exit_copy.gicv3_vmcr = 0;

    exit_copy.cntp_ctl = 0;
    exit_copy.cntp_cval = 0;
    exit_copy.cntv_ctl = 0;
    exit_copy.cntv_cval = 0;

    exit_copy.pmu_ovf_status = 0;

    if (!val_memcmp(&exit_copy, &expected_zero_exit, sizeof(exit_copy))) {
        return VAL_SUCCESS;
    }

    exit_bytes = (uint8_t *)&exit_copy;
    for (i = 0; i + sizeof(uint64_t) <= sizeof(exit_copy); i += sizeof(uint64_t)) {
        val_memcpy(&value, exit_bytes + i, sizeof(uint64_t));
        if (value) {
            LOG(ERROR, "\tUnexpected REC exit word offset 0x%lx\n", (unsigned long)i);
            return VAL_ERROR;
        }
    }

    return VAL_ERROR;
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

static uint64_t valid_input_args_prep_sequence(void)
{
    val_host_pdev_ts pdev_dev;
    val_host_vdev_ts vdev_dev;
    val_host_realm_ts realm;
    val_host_rec_enter_ts *rec_enter;
    val_host_rec_exit_ts *rec_exit;
    val_host_pdev_flags_ts pdev_flags;
    uint64_t ret, i;
    uint32_t rp_bdf, status;
    uint8_t *shared_report_buff = (val_get_shared_region_base() + TEST_USE_OFFSET4);
    uint64_t *shared_addr_ranges = (uint64_t *)(val_get_shared_region_base() +
                                                TEST_USE_OFFSET6);

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

    c_args.rd_valid = realm.rd;
    c_args.rec_ptr_valid = realm.rec[0];
    c_args.pdev_ptr_valid = pdev_dev.pdev;
    c_args.vdev_dev_valid = vdev_dev;
    c_args.pdev_dev_valid = pdev_dev;
    c_args.realm_valid = realm;
    c_args.report_size = vdev_dev.ifc_report_len;

    shared_addr_ranges[0] = pdev_dev.ncoh_num_addr_range;
    for (i = 0; i < pdev_dev.ncoh_num_addr_range; i++)
    {
        shared_addr_ranges[1 + (2 * i)] = pdev_dev.ncoh_addr_range[i].base;
        shared_addr_ranges[2 + (2 * i)] = pdev_dev.ncoh_addr_range[i].top;
    }

    rec_enter = &(((val_host_rec_run_ts *)realm.run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);
    val_memcpy(shared_report_buff, vdev_dev.ifc_report, c_args.report_size);

    ret = val_host_rmi_rec_enter(c_args.realm_valid.rec[0], c_args.realm_valid.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%lx\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL)
    {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx\n",
            rec_exit->exit_reason, rec_exit->esr);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    rec_enter->gprs[1] = vdev_dev.vdev_id;

    ret = val_host_rmi_rec_enter(c_args.realm_valid.rec[0], c_args.realm_valid.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%lx\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (rec_exit->exit_reason != RMI_EXIT_VDEV_VALIDATE_MAPPING) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason,
                                                                        rec_exit->esr);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (validate_rec_exit_zero_fields(rec_exit)) {
        LOG(ERROR, "\tREC exit contains unexpected non-zero fields\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    LOG(DBG, "rec_exit->dev_mem_base %lx rec_exit->dev_mem_top %lx\n", rec_exit->dev_mem_base,
                                                                        rec_exit->dev_mem_top);
    LOG(DBG, "rec_exit->dev_mem_pa %lx\n", rec_exit->dev_mem_pa);

    if (rec_exit->vdev_id_1 != c_args.vdev_dev_valid.vdev_id)
    {
        LOG(ERROR, "Unexpected vdev_id_1: 0x%lx\n", rec_exit->vdev_id_1);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    c_args.base_valid = rec_exit->dev_mem_base;
    c_args.top_valid = rec_exit->dev_mem_top;
    c_args.pa_valid = rec_exit->dev_mem_pa;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;
    val_host_rtt_entry_ts rtte;
    uint64_t ret;

    switch (label)
    {
        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            args->rec_ptr = c_args.rec_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;


        case RD_GRAN_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            args->rec_ptr = c_args.rec_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rec_ptr = g_unaligned_prep_sequence(c_args.rec_ptr_valid);
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;


        case REC_GRAN_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->rec_ptr = g_undelegated_prep_sequence();
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_OWNER:
            args->rd = rd_new_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_TEST_PREP_SEQ_FAILED;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case SIZE_INVALID:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->base = c_args.base_valid;
            args->top = args->base;
            break;

        case BASE_BOUND:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->base = c_args.base_valid + 1U;
            args->top = c_args.top_valid;
            break;

        case TOP_BOUND:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid + PAGE_SIZE;
            break;

        case TOP_GRAN_ALIGN:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->base = c_args.base_valid;
            args->top = g_unaligned_prep_sequence(c_args.top_valid);
            break;

        case BASE_LEVEL_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;

            ret = val_host_rmi_rtt_read_entry(c_args.rd_valid,
                                              c_args.base_valid,
                                              VAL_RTT_MAX_LEVEL,
                                              &rtte);
            if (ret)
                return VAL_TEST_PREP_SEQ_FAILED;

            if (val_host_addr_align_to_level(c_args.base_valid,
                                             rtte.walk_level) ==
                                             c_args.base_valid)
                return VAL_TEST_PREP_SEQ_FAILED;

            test_data->index = rtte.walk_level;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t advance_vdev_measurement_freshness(void)
{
    val_host_vdev_measure_params_ts *params;
    val_smc_param_ts cmd_ret;

    params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (params == NULL)
        return VAL_TEST_PREP_SEQ_FAILED;

    val_memset(params, 0, PAGE_SIZE);
    params->nonce[31] = (unsigned char)1U << 6U;

    cmd_ret = val_host_rmi_vdev_get_measurements(c_args.rd_valid,
                                                 c_args.pdev_ptr_valid,
                                                 c_args.vdev_ptr_valid,
                                                 (uint64_t)params);
    val_host_mem_free(params);
    if (cmd_ret.x0 != RMI_SUCCESS)
    {
        LOG(ERROR, "VDEV_GET_MEASUREMENTS failed, ret=0x%lx\n", cmd_ret.x0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (val_host_dev_communicate(&c_args.realm_valid, &c_args.pdev_dev_valid,
                                 &c_args.vdev_dev_valid, RMI_VDEV_STARTED))
    {
        LOG(ERROR, "VDEV measurement communication failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    return VAL_SUCCESS;
}

void cmd_rtt_dev_validate_host(void)
{
    uint64_t i, ret, out_top;
    val_smc_param_ts cmd_ret;
    struct arguments args;
    val_host_rtt_entry_ts rtte;
    val_host_rec_run_ts *run;
    val_host_rec_enter_flags_ts rec_enter_flags;

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_device;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "\n\tCheck %d : ", i + 1);
        LOG(TEST, test_data[i].msg);
        LOG(TEST, "; intent id : 0x%x \n", test_data[i].label);

        if (intent_to_seq(&test_data[i], &args))
        {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto destroy_device;
        }

        cmd_ret = val_host_rmi_rtt_dev_validate_smc(args.rd, args.rec_ptr,
                                                    args.base, args.top);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_device;
        }
    }

    ret = val_host_dev_mem_map(&c_args.realm_valid,
                               c_args.vdev_ptr_valid,
                               c_args.base_valid,
                               c_args.top_valid,
                               c_args.pa_valid);
    if (ret)
    {
        LOG(ERROR, "val_host_dev_mem_map failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    LOG(TEST, "\n\tCheck vdev_1_freshness\n");
    if (advance_vdev_measurement_freshness() == VAL_TEST_PREP_SEQ_FAILED)
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    cmd_ret = val_host_rmi_rtt_dev_validate_smc(c_args.rd_valid,
                                                 c_args.rec_ptr_valid,
                                                 c_args.base_valid,
                                                 c_args.top_valid);
    if (cmd_ret.x0 != RMI_ERROR_DEVICE)
    {
        LOG(ERROR, "vdev_1_freshness returned 0x%lx, expected 0x%x\n",
            cmd_ret.x0, RMI_ERROR_DEVICE);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    /* Reject the stale request so the Realm can retry with current freshness. */
    run = (val_host_rec_run_ts *)c_args.realm_valid.run[0];
    val_memcpy(&rec_enter_flags, &run->enter.flags, sizeof(rec_enter_flags));
    rec_enter_flags.dev_mem_response = RMI_RESPONSE_REJECT;
    val_memcpy(&run->enter.flags, &rec_enter_flags, sizeof(rec_enter_flags));

    ret = val_host_rmi_rec_enter(c_args.rec_ptr_valid, c_args.realm_valid.run[0]);
    if (ret || (run->exit.exit_reason != RMI_EXIT_VDEV_VALIDATE_MAPPING) ||
        (run->exit.vdev_id_1 != c_args.vdev_dev_valid.vdev_id) ||
        (run->exit.dev_mem_base != c_args.base_valid) ||
        (run->exit.dev_mem_top != c_args.top_valid) ||
        (run->exit.dev_mem_pa != c_args.pa_valid))
    {
        LOG(ERROR, "Realm did not retry the rejected device mapping\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    rec_enter_flags.dev_mem_response = RMI_RESPONSE_ACCEPT;
    val_memcpy(&run->enter.flags, &rec_enter_flags, sizeof(rec_enter_flags));

    LOG(TEST, "Positive Observability Check\n");

    ret = val_host_rmi_rtt_dev_validate(c_args.rd_valid, c_args.rec_ptr_valid,
                                        c_args.base_valid, c_args.top_valid,
                                        &out_top);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "Command failed. %x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_device;
    }

    if (out_top != c_args.top_valid)
    {
        LOG(ERROR, "Unexpected out_top: 0x%lx, expected 0x%lx\n",
            out_top, c_args.top_valid);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_device;
    }

    for (i = c_args.base_valid; i < c_args.top_valid; i += PAGE_SIZE) {
        uint64_t expected_pa = c_args.pa_valid + (i - c_args.base_valid);

        ret = val_host_rmi_rtt_read_entry(c_args.rd_valid, i, VAL_RTT_MAX_LEVEL, &rtte);
        if (ret)
        {
            LOG(ERROR, "rtt_read_entry failed ret = %x, ipa = %lx\n", ret, i);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto destroy_device;
        }

        if ((rtte.state != RMI_ASSIGNED_DEV) || (rtte.ripas != RMI_DEV))
        {
            LOG(ERROR, "Unexpected RTTE states: ipa %lx hipas %d ripas %d\n",
                i, rtte.state, rtte.ripas);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
            goto destroy_device;
        }

        if (OA(rtte.desc) != expected_pa)
        {
            LOG(ERROR, "Unexpected RTTE PA: ipa %lx expected_pa %lx actual_pa %lx\n",
                i, expected_pa, OA(rtte.desc));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
            goto destroy_device;
        }
    }

    ret = val_host_rmi_rec_enter(c_args.realm_valid.rec[0], c_args.realm_valid.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
        goto destroy_device;
    }

    if (IS_TEST_START(val_get_status()))
        val_set_status(RESULT_PASS(VAL_SUCCESS));

destroy_device:
    if (c_args.vdev_ptr_valid &&
        val_host_vdev_teardown(&c_args.realm_valid, &c_args.pdev_dev_valid,
                               &c_args.vdev_dev_valid))
    {
        LOG(ERROR, "VDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto exit;
    }

    if (c_args.pdev_ptr_valid &&
        val_host_pdev_teardown(&c_args.pdev_dev_valid, c_args.pdev_ptr_valid))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto exit;
    }

    if (c_args_invalid.realm_invalid.rd &&
        val_host_realm_destroy(c_args_invalid.realm_invalid.rd))
    {
        LOG(ERROR, "Invalid Realm teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto exit;
    }

exit:
    return;
}
