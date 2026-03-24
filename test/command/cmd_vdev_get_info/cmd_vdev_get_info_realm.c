/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "rsi_vdev_get_info_data.h"
#include "val_realm_framework.h"

#define INVALID_VDEV_ID         0xABC
/*  4KB aligned and Protected IPA to which the configuration will be written */
__attribute__((aligned (PAGE_SIZE))) static uint8_t vdev_get_info[PAGE_SIZE];
val_realm_rsi_host_call_t *realm_flag_host_call1;

static struct argument_store {
    uint64_t vdev_id_valid;
    uint64_t addr_valid;
} c_args;

struct arguments {
    uint64_t vdev_id;
    uint64_t addr;
};

static uint64_t unaligned_prep_sequence(uint64_t addr)
{
    return addr + 1;
}

static uint64_t addr_ripas_empty_prep_sequence(void)
{
    __attribute__((aligned (PAGE_SIZE))) static uint8_t addr_vdev_get_info[PAGE_SIZE];
    val_smc_param_ts args;

    /* Request Host to transition RIPAS from RAM to EMPTY */
    val_memset(&args, 0x0, sizeof(val_smc_param_ts));
    args = val_realm_rsi_ipa_state_set((uint64_t)addr_vdev_get_info,
		    (uint64_t)(addr_vdev_get_info + PAGE_SIZE), RSI_EMPTY, RSI_NO_CHANGE_DESTROYED);
    if (args.x0 || (args.x1 != (uint64_t)(addr_vdev_get_info + PAGE_SIZE)))
    {
        LOG(ERROR, "\trsi_ipa_state_set failed x0 %lx x1 %lx\n", args.x0, args.x1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    args = val_realm_rsi_ipa_state_get((uint64_t)addr_vdev_get_info,
                                    (uint64_t)(addr_vdev_get_info + PAGE_SIZE));
    if ((args.x0 != RSI_SUCCESS) || (args.x2 != RSI_EMPTY) ||
                    (args.x1 != (uint64_t)(addr_vdev_get_info + PAGE_SIZE))) {
        LOG(ERROR, "RSI_IPA_STATE_GET failed, ret 0x%lx out_top=0x%lx ripas=%lx\n",
                                                        args.x0, args.x1, args.x2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
        goto exit;
    }

    return (uint64_t)addr_vdev_get_info;
exit:
    return VAL_TEST_PREP_SEQ_FAILED;
}

static uint64_t address_unprotected_prep_sequence(void)
{
    uint64_t ipa_width;

    ipa_width = val_realm_get_ipa_width();
    return 1ULL << (ipa_width - 1);
}

static uint64_t valid_input_args_prep_sequence(void)
{
    val_realm_rsi_host_call_t *gv_realm_host_call;
    uint64_t vdev_id;
    gv_realm_host_call = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);
    vdev_id = gv_realm_host_call->gprs[1];

    c_args.vdev_id_valid = vdev_id;
    c_args.addr_valid = (uint64_t)vdev_get_info;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case VDEV_IDS_INVALID_VDEV_ID:
            args->vdev_id = INVALID_VDEV_ID;
            args->addr = c_args.addr_valid;
            break;

        case ADDR_UNALIGNED:
            args->vdev_id = c_args.vdev_id_valid;
            args->addr = unaligned_prep_sequence(c_args.addr_valid);
            break;

        case ADDR_UNPROTECTED:
            args->vdev_id = c_args.vdev_id_valid;
            args->addr = address_unprotected_prep_sequence();
            break;

        case ADDR_RIPAS_EMPTY:
            args->vdev_id = c_args.vdev_id_valid;
            args->addr = addr_ripas_empty_prep_sequence();
            if (args->addr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_TEST_PREP_SEQ_FAILED;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_vdev_get_info_realm(void)
{
    uint64_t i;
    val_smc_param_ts cmd_ret;
    struct arguments args;

    realm_flag_host_call1 = val_realm_rsi_host_call_ripas(VAL_SWITCH_TO_HOST);

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    for (i = 0; i < (sizeof(test_data1) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "\n\tCheck %d : ", i + 1);
        LOG(TEST, test_data1[i].msg);
        LOG(TEST, "; intent id : 0x%x \n", test_data1[i].label);

        if (intent_to_seq(&test_data1[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }

        cmd_ret = val_realm_rsi_vdev_get_info(args.vdev_id, args.addr);
        if (cmd_ret.x0 != PACK_CODE(test_data1[i].status, test_data1[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data1[i].status, test_data1[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto exit;
        }
    }

    /* Go back to the host once all checks are done for Realm-A */
    val_realm_return_to_host();

    LOG(TEST, "\n\tPositive Observability Check\n");
    cmd_ret = val_realm_rsi_vdev_get_info(c_args.vdev_id_valid, c_args.addr_valid);
    if (cmd_ret.x0 != 0)
    {
        LOG(ERROR, "Command failed. %x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    val_realm_rsi_vdev_info_ts *rsi_device_info =
                        (val_realm_rsi_vdev_info_ts *)c_args.addr_valid;
    /* Check cfg.hash_algo as pdev.hash_algo, here it is RMI_HASH_SHA_256*/
    if (rsi_device_info->hash_algo != RMI_HASH_SHA_256)
    {
        LOG(ERROR, "Hash algo mismatched from PDEV hash algo, ret %x\n",
                                              rsi_device_info->hash_algo);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    val_realm_return_to_host();
}
