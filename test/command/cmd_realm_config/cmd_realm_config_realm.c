/*
 * Copyright (c) 2023, 2025-2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_realm_rsi.h"
#include "val_realm_framework.h"
#include "rsi_realm_config_data.h"

#define IPA_ADDR_PROTECTED_UNASSIGNED_EMPTY    (3 * PAGE_SIZE)

/*  4KB aligned and Protected IPA to which realm configuration will be written */
__attribute__((aligned (PAGE_SIZE)))
static val_realm_rsi_realm_config_ts realm_config;

static struct argument_store {
    uint64_t addr_valid;
} c_args;

struct arguments {
    uint64_t addr;
};

static void valid_argument_prep_sequence(void)
{
    c_args.addr_valid = (uint64_t)&realm_config;
}

static uint64_t unaligned_prep_sequence(uint64_t addr)
{
    return addr + 1;
}

static uint64_t address_unprotected_prep_sequence(void)
{
    uint64_t ipa_width;

    ipa_width = val_realm_get_ipa_width();
    return 1ULL << (ipa_width - 1);
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    switch (label)
    {
        case ADDR_UNALIGNED:
            args->addr = unaligned_prep_sequence(c_args.addr_valid);
            break;

        case ADDR_UNPROTECTED:
            args->addr = address_unprotected_prep_sequence();
            break;

        case ADDR_EMPTY:
            args->addr = IPA_ADDR_PROTECTED_UNASSIGNED_EMPTY;
            break;

        default:
            // set status to failure
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_realm_config_realm(void)
{
    uint64_t ret;
    struct arguments args;
    uint64_t i;
    uint8_t *config = (uint8_t *)&realm_config;

    /* Prepare valid arguments */
    valid_argument_prep_sequence();

    /* Iterate over the input */
    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
              i + 1, test_data[i].msg, test_data[i].label);

        if (intent_to_seq(&test_data[i], &args)) {
            LOG(ERROR, "Intent to sequence failed\n");
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
            goto exit;
        }

        ret = val_realm_rsi_realm_config(args.addr);

        if (ret != test_data[i].status)
        {
            LOG(ERROR, "Unexpected Command Return Status ret status : 0x%x \n", ret);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);

    val_memset(&realm_config, 0xFF, sizeof(realm_config));
    ret = val_realm_rsi_realm_config(c_args.addr_valid);
    if (ret)
    {
        LOG(ERROR, "Positive Observability failed. Ret = 0x%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto exit;
    }

    if (realm_config.ipa_width != val_realm_get_ipa_width())
    {
        LOG(ERROR, "IPA width mismatch, expected=0x%lx returned=0x%lx\n",
            val_realm_get_ipa_width(), realm_config.ipa_width);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto exit;
    }

    if (realm_config.hash_algo != RSI_HASH_SHA_256)
    {
        LOG(ERROR, "Hash algorithm mismatch, expected=0x%x returned=0x%x\n",
            RSI_HASH_SHA_256, realm_config.hash_algo);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    if (realm_config.num_aux_planes != 0)
    {
        LOG(ERROR, "Number of auxiliary Planes mismatch, expected=0 returned=0x%lx\n",
            realm_config.num_aux_planes);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    if (realm_config.ats_plane != 0)
    {
        LOG(ERROR, "ATS Plane mismatch, expected=0 returned=0x%lx\n",
            realm_config.ats_plane);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    /* The 64-byte RPV ends at 0x240; the remaining bytes are reserved. */
    for (i = 0x240; i < PAGE_SIZE; i++)
    {
        if (config[i] != 0)
        {
            LOG(ERROR, "Reserved byte at offset 0x%lx is nonzero: 0x%x\n",
                i, config[i]);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
            goto exit;
        }
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    val_realm_return_to_host();
}
