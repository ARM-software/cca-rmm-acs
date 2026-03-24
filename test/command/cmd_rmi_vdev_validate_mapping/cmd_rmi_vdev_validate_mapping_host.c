/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_vdev_validate_mapping_data.h"
#include "command_common_host.h"
#include "val_host_pcie.h"
#include "val_host_helpers.h"

#define VALID_REALM 1

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
    uint64_t rd_invalid;
    uint64_t pdev_invalid;
    val_host_pdev_ts pdev_dev_invalid;
} c_args_invalid;

struct arguments {
    uint64_t rd;
    uint64_t rec_ptr;
    uint64_t pdev_ptr;
    uint64_t vdev_ptr;
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

static uint64_t invalid_pdev_prep_sequence(void)
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

    pdev_dev.pdev = g_pdev_ready_prep_sequence(&pdev_dev);
    if (pdev_dev.pdev == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args_invalid.pdev_invalid = pdev_dev.pdev;
    c_args_invalid.pdev_dev_invalid = pdev_dev;

    return pdev_dev.pdev;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    val_host_pdev_ts pdev_dev;
    val_host_vdev_ts vdev_dev;
    val_host_realm_ts realm;
    val_host_rec_enter_ts *rec_enter;
    val_host_rec_exit_ts *rec_exit;
    val_host_pdev_flags_ts pdev_flags;
    val_smc_param_ts args;
    uint64_t ret;
    uint32_t rp_bdf, status;
    uint8_t *shared_report_buff = (val_get_shared_region_base() + TEST_USE_OFFSET4);

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

    if (rec_exit->exit_reason != RMI_EXIT_VDEV_REQUEST)
    {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx\n",
            rec_exit->exit_reason, rec_exit->esr);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    args = val_host_rmi_vdev_complete(c_args.realm_valid.rec[0], c_args.vdev_ptr_valid);
    if (args.x0)
    {
        LOG(ERROR, "\tVDEV complete failed, ret=%lx\n", args.x0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Check that REC Exit was due to RIPAS Change request */
    if (rec_exit->exit_reason != RMI_EXIT_VDEV_REQUEST) {
        LOG(ERROR, "\tUnexcpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason,
                                                                        rec_exit->esr);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    /* Complete the VDEV request */
    args = val_host_rmi_vdev_complete(c_args.realm_valid.rec[0], c_args.vdev_ptr_valid);
    if (args.x0)
    {
        LOG(ERROR, "\tVDEV complete failed, ret=%lx\n", args.x0);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret);
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    if (rec_exit->exit_reason != RMI_EXIT_VDEV_MAP) {
        LOG(ERROR, "\tUnexcpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason,
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

    ret = val_host_dev_mem_map(&c_args.realm_valid, c_args.vdev_ptr_valid, rec_exit->dev_mem_base,
                        rec_exit->dev_mem_top, rec_exit->dev_mem_pa);
    if (ret)
    {
        LOG(ERROR, "val_host_dev_mem_map failed \n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

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

    switch (label)
    {
        case RD_UNALIGNED:
            args->rd = g_unaligned_prep_sequence(c_args.rd_valid);
            args->rec_ptr = c_args.rec_ptr_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_OUTSIDE_OF_PERMITTED_PA:
            args->rd = g_outside_of_permitted_pa_prep_sequence();
            args->rec_ptr = c_args.rec_ptr_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_DEV_MEM_MMIO:
            args->rd = g_dev_mem_prep_sequence();
            args->rec_ptr = c_args.rec_ptr_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case RD_GRAN_STATE_UNDELEGATED:
            args->rd = g_undelegated_prep_sequence();
            args->rec_ptr = c_args.rec_ptr_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rec_ptr = g_unaligned_prep_sequence(c_args.rec_ptr_valid);
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_OUTSIDE_OF_PERMITTED_PA:
            args->rd = c_args.rd_valid;
            args->rec_ptr = g_outside_of_permitted_pa_prep_sequence();
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_DEV_MEM_MMIO:
            args->rd = c_args.rd_valid;
            args->rec_ptr = g_dev_mem_prep_sequence();
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_GRAN_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->rec_ptr = g_undelegated_prep_sequence();
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case REC_OWNER:
            args->rd = rd_new_prep_sequence();
            if (args->rd == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_TEST_PREP_SEQ_FAILED;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case PDEV_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->pdev_ptr = g_unaligned_prep_sequence(c_args.pdev_ptr_valid);
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case PDEV_OUTSIDE_OF_PERMITTED_PA:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->pdev_ptr = g_outside_of_permitted_pa_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case PDEV_DEV_MEM_MMIO:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->pdev_ptr = g_dev_mem_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case PDEV_GRAN_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->pdev_ptr = g_undelegated_prep_sequence();
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case VDEV_UNALIGNED:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = g_unaligned_prep_sequence(c_args.vdev_ptr_valid);
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case VDEV_OUTSIDE_OF_PERMITTED_PA:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = g_outside_of_permitted_pa_prep_sequence();
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case VDEV_DEV_MEM_MMIO:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = g_dev_mem_prep_sequence();
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case VDEV_GRAN_STATE_UNDELEGATED:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = g_undelegated_prep_sequence();
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case VDEV_PDEV:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->pdev_ptr = invalid_pdev_prep_sequence();
            if (args->pdev_ptr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_TEST_PREP_SEQ_FAILED;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid;
            break;

        case SIZE_INVALID:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->base = c_args.base_valid;
            args->top = args->base;
            break;

        case BASE_BOUND:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->base = c_args.base_valid + 1U;
            args->top = c_args.top_valid;
            break;

        case TOP_BOUND:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->base = c_args.base_valid;
            args->top = c_args.top_valid + PAGE_SIZE;
            break;

        case TOP_GRAN_ALIGN:
            args->rd = c_args.rd_valid;
            args->rec_ptr = c_args.rec_ptr_valid;
            args->pdev_ptr = c_args.pdev_ptr_valid;
            args->vdev_ptr = c_args.vdev_ptr_valid;
            args->base = c_args.base_valid;
            args->top = g_unaligned_prep_sequence(c_args.top_valid);
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_rmi_vdev_validate_mapping_host(void)
{
    uint64_t i, ret;
    val_smc_param_ts cmd_ret;
    struct arguments args;
    val_host_rtt_entry_ts rtte;

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

        cmd_ret = val_host_rmi_vdev_validate_mapping(args.rd, args.rec_ptr, args.pdev_ptr,
                                                     args.vdev_ptr, args.base, args.top);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto destroy_device;
        }
    }

    LOG(TEST, "Positive Observability Check\n");

    cmd_ret = val_host_rmi_vdev_validate_mapping(c_args.rd_valid, c_args.rec_ptr_valid,
         c_args.pdev_ptr_valid, c_args.vdev_ptr_valid, c_args.base_valid, c_args.top_valid);
    if (cmd_ret.x0 != 0)
    {
        LOG(ERROR, "Command failed. %x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
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

    if (c_args_invalid.pdev_invalid &&
        val_host_pdev_teardown(&c_args_invalid.pdev_dev_invalid, c_args_invalid.pdev_invalid))
    {
        LOG(ERROR, "PDEV teardown failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
        goto exit;
    }

exit:
    return;
}
