/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "rsi_vdev_validate_mapping_data.h"
#include "val_realm_framework.h"
#include "val_realm_device_assignment.h"

#define INVALID_VDEV_ID         0xABC
#define PAGE_SIZE_ALIGNED __attribute__((aligned (PAGE_SIZE)))

static struct argument_store {
    uint64_t vdev_id_valid;
    uint64_t ipa_base_valid;
    uint64_t ipa_top_valid;
    uint64_t pa_base_valid;
    uint64_t flags_valid;
    uint64_t lock_nonce_valid;
    uint64_t meas_nonce_valid;
    uint64_t report_nonce_valid;
} c_args;

struct arguments {
    uint64_t vdev_id;
    uint64_t ipa_base;
    uint64_t ipa_top;
    uint64_t pa_base;
    uint64_t flags;
    uint64_t lock_nonce;
    uint64_t meas_nonce;
    uint64_t report_nonce;
};

static uint64_t unaligned_prep_sequence(uint64_t addr)
{
    return addr + 1U;
}

static uint64_t address_unprotected_prep_sequence(void)
{
    uint64_t ipa_width;

    ipa_width = val_realm_get_ipa_width();
    return 1ULL << (ipa_width - 1);
}

static uint64_t valid_input_args_prep_sequence(void)
{
    val_smc_param_ts args;
    val_realm_rsi_host_call_t *gv_realm_host_call;
    val_realm_rsi_dev_mem_flags_ts dev_mem_flags;
    uint64_t flags;
    uint64_t vdev_id;
    uint8_t *shared_report_buff = (val_get_shared_region_base() + TEST_USE_OFFSET4);
    PAGE_SIZE_ALIGNED val_host_pci_tdisp_device_interface_report_struct_t interface_report = {0};
    pci_tdisp_mmio_range_t mmio_range_struct[MAX_MMIO_RANGE_COUNT];
    PAGE_SIZE_ALIGNED val_realm_rsi_vdev_info_ts vdev_info = {0};

    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    vdev_id = gv_realm_host_call->gprs[1];

    args = val_realm_rsi_vdev_get_info(vdev_id, (uint64_t)&vdev_info);
    if (args.x0)
    {
        LOG(ERROR, "VDEV_GET_INFO failed, ret %x\n", args.x0);
        goto exit;
    }

    if (interface_report_decoding(shared_report_buff, &interface_report,
                                                    mmio_range_struct))
    {
        LOG(ERROR, "Interface report decoding Failed\n");
        goto exit;
    }

    if (interface_report.mmio_range_count == 0) {
        LOG(ERROR, "Interface report has no MMIO ranges\n");
        goto exit;
    }

    val_memset(&dev_mem_flags, 0, sizeof(dev_mem_flags));
    dev_mem_flags.coh = RSI_DEV_MEM_NON_COHERENT;
    dev_mem_flags.order = RSI_DEV_MEM_NOT_LIMITED_ORDER;
    val_memcpy(&flags, &dev_mem_flags, sizeof(dev_mem_flags));

    c_args.vdev_id_valid = vdev_id;
    c_args.ipa_base_valid = mmio_range_struct[0].first_page;
    c_args.ipa_top_valid = c_args.ipa_base_valid +
                           (PAGE_SIZE_4K * mmio_range_struct[0].number_of_pages);
    c_args.pa_base_valid = c_args.ipa_base_valid;
    c_args.flags_valid = flags;
    c_args.lock_nonce_valid = vdev_info.lock_nonce;
    c_args.meas_nonce_valid = vdev_info.meas_nonce;
    c_args.report_nonce_valid = vdev_info.report_nonce;

    return VAL_SUCCESS;

exit:
    return VAL_TEST_PREP_SEQ_FAILED;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case VDEV_IDS_INVALID_VDEV_ID:
            args->vdev_id = INVALID_VDEV_ID;
            args->ipa_base = c_args.ipa_base_valid;
            args->ipa_top = c_args.ipa_top_valid;
            args->pa_base = c_args.pa_base_valid;
            args->flags = c_args.flags_valid;
            args->lock_nonce = c_args.lock_nonce_valid;
            args->meas_nonce = c_args.meas_nonce_valid;
            args->report_nonce = c_args.report_nonce_valid;
            break;

        case IPA_BASE_UNALIGNED:
            args->vdev_id = c_args.vdev_id_valid;
            args->ipa_base = unaligned_prep_sequence(c_args.ipa_base_valid);
            args->ipa_top = c_args.ipa_top_valid;
            args->pa_base = c_args.pa_base_valid;
            args->flags = c_args.flags_valid;
            args->lock_nonce = c_args.lock_nonce_valid;
            args->meas_nonce = c_args.meas_nonce_valid;
            args->report_nonce = c_args.report_nonce_valid;
            break;

        case IPA_TOP_UNALIGNED:
            args->vdev_id = c_args.vdev_id_valid;
            args->ipa_base = c_args.ipa_base_valid;
            args->ipa_top = unaligned_prep_sequence(c_args.ipa_top_valid);
            args->pa_base = c_args.pa_base_valid;
            args->flags = c_args.flags_valid;
            args->lock_nonce = c_args.lock_nonce_valid;
            args->meas_nonce = c_args.meas_nonce_valid;
            args->report_nonce = c_args.report_nonce_valid;
            break;

        case PA_UNALIGNED:
            args->vdev_id = c_args.vdev_id_valid;
            args->ipa_base = c_args.ipa_base_valid;
            args->ipa_top = c_args.ipa_top_valid;
            args->pa_base = unaligned_prep_sequence(c_args.pa_base_valid);
            args->flags = c_args.flags_valid;
            args->lock_nonce = c_args.lock_nonce_valid;
            args->meas_nonce = c_args.meas_nonce_valid;
            args->report_nonce = c_args.report_nonce_valid;
            break;

        case SIZE_INVALID:
            args->vdev_id = c_args.vdev_id_valid;
            args->ipa_base = c_args.ipa_base_valid;
            args->ipa_top = c_args.ipa_base_valid;
            args->pa_base = c_args.pa_base_valid;
            args->flags = c_args.flags_valid;
            args->lock_nonce = c_args.lock_nonce_valid;
            args->meas_nonce = c_args.meas_nonce_valid;
            args->report_nonce = c_args.report_nonce_valid;
            break;

        case REGION_UNPROTECTED:
            args->vdev_id = c_args.vdev_id_valid;
            args->ipa_base = address_unprotected_prep_sequence();
            args->ipa_top = args->ipa_base + PAGE_SIZE;
            args->pa_base = args->ipa_base;
            args->flags = c_args.flags_valid;
            args->lock_nonce = c_args.lock_nonce_valid;
            args->meas_nonce = c_args.meas_nonce_valid;
            args->report_nonce = c_args.report_nonce_valid;
            break;

        case ATTEST_INFO_MISMATCH:
            args->vdev_id = c_args.vdev_id_valid;
            args->ipa_base = c_args.ipa_base_valid;
            args->ipa_top = c_args.ipa_top_valid;
            args->pa_base = c_args.pa_base_valid;
            args->flags = c_args.flags_valid;
            args->lock_nonce = c_args.lock_nonce_valid + 1U;
            args->meas_nonce = c_args.meas_nonce_valid;
            args->report_nonce = c_args.report_nonce_valid;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_rsi_vdev_validate_mapping_realm(void)
{
    uint64_t i;
    val_smc_param_ts cmd_ret;
    struct arguments args;

    val_realm_return_to_host();

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    for (i = 0; i < (sizeof(test_data1) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "\n\tCheck %d : ", i + 1);
        LOG(TEST, test_data1[i].msg);
        LOG(TEST, "; intent id : 0x%x \n", test_data1[i].label);

        if (intent_to_seq(&test_data1[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        cmd_ret = val_realm_rsi_vdev_validate_mapping(args.vdev_id,
                        args.ipa_base, args.ipa_top, args.pa_base,
                        args.flags, args.lock_nonce,
                        args.meas_nonce, args.report_nonce);

        if (cmd_ret.x0 != PACK_CODE(test_data1[i].status, test_data1[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data1[i].status, test_data1[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }
    /* Go back to the host once all checks are done for Realm-A */
    val_realm_return_to_host();

    LOG(TEST, "Positive Observability Check\n");
    args.ipa_base = c_args.ipa_base_valid;
    args.ipa_top = c_args.ipa_top_valid;
    args.pa_base = c_args.pa_base_valid;
    do {
        cmd_ret = val_realm_rsi_vdev_validate_mapping(c_args.vdev_id_valid,
                        args.ipa_base, c_args.ipa_top_valid, c_args.pa_base_valid,
                        c_args.flags_valid, c_args.lock_nonce_valid,
                        c_args.meas_nonce_valid, c_args.report_nonce_valid);
        if (cmd_ret.x0 != RSI_SUCCESS)
        {
            LOG(ERROR, "Command failed. %x\n", cmd_ret.x0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }
        if (cmd_ret.x2 == RSI_REJECT) {
            LOG(TEST, "Host rejected mapping validation. Response=0x%lx\n", cmd_ret.x2);
            val_set_status(RESULT_PASS(VAL_SUCCESS));
            goto exit;
        }
        if (cmd_ret.x2 != RSI_ACCEPT) {
            LOG(ERROR, "Unexpected response. new_base=0x%lx resp=0x%lx\n",
                cmd_ret.x1, cmd_ret.x2);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto exit;
        }
        if ((cmd_ret.x1 == args.ipa_base) && (cmd_ret.x1 != c_args.ipa_top_valid)) {
            LOG(ERROR, "No progress in validation. new_base=0x%lx\n", cmd_ret.x1);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
            goto exit;
        }
        args.ipa_base = cmd_ret.x1;
    } while (args.ipa_base < c_args.ipa_top_valid);

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    val_realm_return_to_host();
}
