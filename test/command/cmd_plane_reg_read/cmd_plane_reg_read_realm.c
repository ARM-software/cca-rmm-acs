/*
 * Copyright (c) 2024-2025, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "rsi_plane_reg_read_data.h"
#include "val_realm_framework.h"
#include "val_realm_planes.h"

#define PLANE_IDX_1                     1
#define PLANE_IDX_OUT_OF_BOUND          2

static struct argument_store {
    uint64_t plane_index_valid;
    uint64_t encoding_valid;
} c_args;

struct arguments {
    uint64_t plane_index;
    uint64_t encoding;
};

static uint64_t encoding_valid_prep_sequence(void)
{
    uint64_t p1_ipa_base, p1_ipa_top, esr;
    __attribute__((aligned (PAGE_SIZE))) val_realm_rsi_plane_run_ts run_ptr = {0};

    /* Configure Permissions for Plane 1 image */
    p1_ipa_base = VAL_PLANE1_IMAGE_BASE_IPA;
    p1_ipa_top = p1_ipa_base + PLATFORM_REALM_IMAGE_SIZE;

    if (val_realm_plane_perm_init(PLANE_1_INDEX, PLANE_1_PERMISSION_INDEX, p1_ipa_base,
                                                                             p1_ipa_top))
    {
        LOG(ERROR, "Secondary plane permission initialization failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    run_ptr.enter.pc =  VAL_PLANE1_IMAGE_BASE_IPA;

    if (val_realm_run_plane(PLANE_1_INDEX, &run_ptr))
    {
        LOG(ERROR, "Plane run failed failed\n");
        return VAL_TEST_PREP_SEQ_FAILED;

    }

    esr = run_ptr.exit.esr_el2;

    /* Check that Plane exit was due to P0 HVC call*/
    if (run_ptr.exit.reason != RSI_EXIT_SYNC ||
        ESR_EL2_EC(esr) != ESR_EL2_EC_HVC ||
        run_ptr.exit.gprs[0] != PSI_P0_CALL)
    {
        LOG(ERROR, "Invalid exit type: %d, ESR: 0x%lx\n",
                                             run_ptr.exit.reason, run_ptr.exit.esr_el2);
        return VAL_TEST_PREP_SEQ_FAILED;

    }

    return SYSREG_SCTLR_EL1;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    c_args.plane_index_valid = PLANE_IDX_1;

    c_args.encoding_valid = encoding_valid_prep_sequence();
    if (c_args.encoding_valid == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case PLANE_INDEX_OUT_OF_BOUND:
            args->plane_index = PLANE_IDX_OUT_OF_BOUND;
            args->encoding = c_args.encoding_valid;
            break;

        case REG_ENCODING_INVALID:
            args->plane_index = c_args.plane_index_valid;
            args->encoding = SYSREG_SCTLR_EL3;
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static void p0_payload(void)
{
    uint64_t i;
    val_smc_param_ts cmd_ret;
    struct arguments args;

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
              i + 1, test_data[i].msg, test_data[i].label);

        if (intent_to_seq(&test_data[i], &args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        cmd_ret = val_realm_rsi_plane_reg_read(args.plane_index, args.encoding);
        if (cmd_ret.x0 != PACK_CODE(test_data[i].status, test_data[i].index)) {
            LOG(ERROR, "Test Failure!The ABI call returned: %xExpected: %x\n",
                cmd_ret.x0, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);
    cmd_ret = val_realm_rsi_plane_reg_read(c_args.plane_index_valid, c_args.encoding_valid);
    if (cmd_ret.x0 != 0)
    {
        LOG(ERROR, " Command failed. %x\n", cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    /* Check if Alignment check is enabled in SCTLR_EL1*/
    if (VAL_EXTRACT_BITS(cmd_ret.x1, 1, 1) != 0x1)
    {
        LOG(ERROR, " Unexpected Register value. %x\n", cmd_ret.x1);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    val_realm_return_to_host();
}

static void p1_payload(void)
{
    uint64_t currentEL, sctlr;

    currentEL = (val_read_current_el() & 0xc) >> 2;
    sctlr = val_sctlr_read(currentEL);

    /* Enable Alignment check in SCTLR_EL1 */
    sctlr |= (1ULL << 1);
    val_sctlr_write(sctlr, currentEL);

    /* Ensure the affect takes place before we exit plane */
    asm volatile ("isb");

    val_realm_return_to_p0();
}

void cmd_plane_reg_read_realm(void)
{
    if (val_realm_in_p0())
        p0_payload();
    else
        p1_payload();
}

