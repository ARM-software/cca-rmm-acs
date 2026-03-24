/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "rsi_vdev_dma_disable_data.h"
#include "val_realm_framework.h"

#define INVALID_VDEV_ID         0xABC
#define PAGE_SIZE_ALIGNED __attribute__((aligned (PAGE_SIZE)))

static struct argument_store {
    uint64_t vdev_id_valid;
    uint64_t non_ats_plane_valid;
    uint64_t lock_nonce_valid;
    uint64_t meas_nonce_valid;
    uint64_t report_nonce_valid;
} c_args;

struct arguments {
    uint64_t vdev_id;
};

static uint64_t valid_input_args_prep_sequence(void)
{
    val_smc_param_ts args;
    uint64_t vdev_id;
    uint64_t non_ats_plane;
    val_realm_rsi_host_call_t *gv_realm_host_call;
    PAGE_SIZE_ALIGNED val_realm_rsi_vdev_info_ts vdev_info = {0};
    PAGE_SIZE_ALIGNED val_realm_rsi_realm_config_ts realm_config = {0};

    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    vdev_id = gv_realm_host_call->gprs[1];

    args = val_realm_rsi_vdev_get_info(vdev_id, (uint64_t)&vdev_info);
    if (args.x0)
    {
        LOG(ERROR, "VDEV_GET_INFO failed, ret %x\n", args.x0);
        goto exit;
    }

    if (val_realm_rsi_realm_config((uint64_t)(&realm_config)) != RSI_SUCCESS)
    {
        LOG(ERROR, "RSI_REALM_CONFIG failed\n");
        goto exit;
    }

    non_ats_plane = realm_config.ats_plane + 1UL;
    if (non_ats_plane > realm_config.num_aux_planes) {
        non_ats_plane = 0UL;
    }

    args = val_realm_rsi_vdev_dma_enable(vdev_id, 0, non_ats_plane,
                    vdev_info.lock_nonce,
                    vdev_info.meas_nonce,
                    vdev_info.report_nonce);
    if (args.x0)
    {
        LOG(ERROR, "RSI_VDEV_DMA_ENABLE failed, ret %x\n", args.x0);
        goto exit;
    }

    c_args.vdev_id_valid = vdev_id;
    c_args.non_ats_plane_valid = non_ats_plane;
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
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_vdev_dma_disable_realm(void)
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

        cmd_ret = val_realm_rsi_vdev_dma_disable(args.vdev_id);

        if (cmd_ret.x0 != PACK_CODE(test_data1[i].status, test_data1[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data1[i].status, test_data1[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }
    /* Go back to the host once all checks are done for Realm-A */
    val_realm_return_to_host();

    LOG(TEST, "\n\tPositive Observability Check\n");
    cmd_ret = val_realm_rsi_vdev_dma_disable(c_args.vdev_id_valid);
    if (cmd_ret.x0 != 0)
    {
        LOG(ERROR, "Command failed. %x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    val_realm_return_to_host();
}
