/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_psmmu_irq_notify_data.h"
#include "command_common_host.h"
#include "psmmu_command_common_host.h"

#define PSMMU_IRQ_NOTIFY_MAX_LOOPS  (16U)
#define PSMMU_IRQ_PENDING(x)        ((x) & 0x1U)

static struct argument_store {
    uint64_t psmmu_ptr_valid;
    psmmu_cmd_context_ts ctx;
} c_args;

struct arguments {
    uint64_t psmmu_ptr;
    uint64_t irq_set;
};

static uint64_t valid_input_args_prep_sequence(void)
{
    if (psmmu_cmd_context_init(&c_args.ctx) != VAL_SUCCESS) {
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    c_args.psmmu_ptr_valid = c_args.ctx.psmmu;
    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    args->psmmu_ptr = c_args.psmmu_ptr_valid;
    args->irq_set = 0;

    switch (label) {
    case psmmu_valid:
        args->psmmu_ptr = g_unaligned_prep_sequence(c_args.psmmu_ptr_valid);
        break;
    case psmmu_state:
        /* Keep the valid PSMMU inactive to trigger psmmu.state != PSMMU_ACTIVE. */
        break;
    default:
        LOG(ERROR, "Unknown intent label encountered\n");
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_psmmu_irq_notify_host(void)
{
    uint64_t i;
    uint64_t ret;
    uint32_t loop = 0;
    val_smc_param_ts cmd_ret;
    struct arguments args;

    if (!val_host_rmm_supports_da()) {
        LOG(ALWAYS, "DA feature not supported\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        return;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++) {
        LOG(TEST, "\n\tCheck %d : ", i + 1);
        LOG(TEST, "%s", test_data[i].msg);
        LOG(TEST, "; intent id : 0x%x \n", test_data[i].label);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto cleanup;
        }

        cmd_ret = val_host_rmi_psmmu_irq_notify(args.psmmu_ptr, args.irq_set);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "\tTest Failure!\n\tThe ABI call returned: %x\n\tExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto cleanup;
        }
    }

    ret = psmmu_cmd_params_prep_sequence(&c_args.ctx,
                                         PSMMU_CMD_PARAMS_VALID);
    if (ret == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto cleanup;
    }

    ret = val_host_rmi_psmmu_activate(c_args.ctx.psmmu, ret);
    if ((ret == RMI_INCOMPLETE) || (ret != RMI_SUCCESS)) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto cleanup;
    }
    c_args.ctx.activated = 1U;

    LOG(TEST, "\n\tPositive Observability Check\n");
    do {
        cmd_ret = val_host_rmi_psmmu_irq_notify(c_args.psmmu_ptr_valid, 0);
        if (cmd_ret.x0 != RMI_SUCCESS) {
            LOG(ERROR, "RMI_PSMMU_IRQ_NOTIFY command failed. %x\n", cmd_ret.x0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
            goto cleanup;
        }
        if (PSMMU_IRQ_PENDING(cmd_ret.x1) == 0U) {
            break;
        }
        loop++;
    } while (loop < PSMMU_IRQ_NOTIFY_MAX_LOOPS);

    if (loop >= PSMMU_IRQ_NOTIFY_MAX_LOOPS) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto cleanup;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

cleanup:
    psmmu_cmd_context_cleanup(&c_args.ctx);
}
