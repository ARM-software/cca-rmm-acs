/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "rmi_gpt_l1_create_data.h"
#include "val_host_sro.h"
#include "command_common_host.h"

static struct argument_store {
    uint64_t addr_valid;
    uint64_t alloc_valid;
    uint64_t l0gpt_size;
    uint64_t entry_state_created;
} c_args;

struct arguments {
    uint64_t addr;
};

static uint64_t l0gpt_size_prep_sequence(void)
{
    uint64_t feature;
    uint64_t l0gptsz;

    if (val_host_rmi_features(RMI_FEATURE_REGISTER_1_INDEX, &feature))
        return VAL_TEST_PREP_SEQ_FAILED;

    l0gptsz = VAL_EXTRACT_BITS(feature, RMI_FEATURE_REGISTER_1_L0GPTSZ_START,
                               RMI_FEATURE_REGISTER_1_L0GPTSZ_END);

    return 1ULL << (RMI_L0GPTSZ_BASE_SHIFT + l0gptsz);
}

static uint64_t addr_bound_prep_sequence(uint64_t l0gpt_size)
{
    uint64_t addr = g_outside_of_permitted_pa_prep_sequence();

    return ADDR_ALIGN_DOWN(addr, l0gpt_size);
}

static uint64_t unfold_denied_prep_sequence(uint64_t l0gpt_size)
{
    /* The UART region is a platform-dependent candidate for denied unfolding. */
    uint64_t addr = g_dev_mem_prep_sequence();

    return ADDR_ALIGN_DOWN(addr, l0gpt_size);
}

static uint64_t entry_state_prep_sequence(uint64_t addr)
{
    if (val_host_rmi_gpt_l1_create(addr) != RMI_SUCCESS)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.entry_state_created = 1;
    return addr;
}

static uint64_t entry_state_cleanup_sequence(void)
{
    if (!c_args.entry_state_created)
        return VAL_SUCCESS;

    if (val_host_rmi_gpt_l1_destroy(c_args.addr_valid) != RMI_SUCCESS)
        return VAL_ERROR;

    c_args.entry_state_created = 0;
    return VAL_SUCCESS;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    uint64_t l0gpt_size;

    l0gpt_size = l0gpt_size_prep_sequence();
    if (l0gpt_size == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_TEST_PREP_SEQ_FAILED;

    c_args.alloc_valid = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (c_args.alloc_valid == 0)
    {
        LOG(ERROR, "Failed to allocate memory for GPT L1 create\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    c_args.l0gpt_size = l0gpt_size;
    c_args.addr_valid = ADDR_ALIGN_DOWN(c_args.alloc_valid, l0gpt_size);

    return VAL_SUCCESS;
}

static uint64_t gran_invalid_donation_sequence(uint64_t addr, uint64_t *result)
{
    val_smc_param_ts create, donate, cmd_ret, cancel;
    uint64_t *list = NULL;
    uint64_t handle = 0;
    uint64_t flags, count, base, range_size;
    uint64_t status, cleanup_ret, i;
    uint64_t ret = VAL_ERROR;
    bool active = false;

    base = PLATFORM_GPT_L1_INVALID_DONATION_BASE;
    range_size = PLATFORM_GPT_L1_INVALID_DONATION_SIZE;
    if ((base == 0U) || ((base & (PAGE_SIZE - 1U)) != 0U) ||
        (range_size < PAGE_SIZE))
    {
        LOG(ERROR, "No reserved non-DRAM GPT donation range is configured\n");
        return VAL_TEST_PREP_SEQ_FAILED;
    }

    create = val_host_rmi_gpt_l1_create_smc(addr);
    if ((RMI_STATUS(create.x0) != RMI_INCOMPLETE) ||
        (VAL_HOST_SRO_MEM_REQ(create.x0) != RMI_OP_MEM_REQ_DONATE))
    {
        LOG(ERROR, "GPT_L1_CREATE did not request donated memory, ret=0x%x\n",
            create.x0);
        return VAL_ERROR;
    }

    handle = create.x1;
    active = true;
    flags = create.x2 & 0xFFU;
    count = VAL_EXTRACT_BITS(create.x2, 8, 21);
    if (((flags & 0x3U) != RMI_PAGE_L3) || (count == 0U) ||
        (count > VAL_HOST_SRO_DESC_MAX_COUNT) ||
        (count > (range_size / PAGE_SIZE)) ||
        (range_size > (1ULL << 50)) ||
        (base > ((1ULL << 50) - range_size)) ||
        ((VAL_EXTRACT_BITS(flags, 2, 2) == RMI_OP_MEM_CONTIG) &&
         ((count & (count - 1U)) != 0U)) ||
        ((VAL_EXTRACT_BITS(flags, 2, 2) == RMI_OP_MEM_CONTIG) &&
         ((base & ((count * PAGE_SIZE) - 1U)) != 0U)))
    {
        LOG(ERROR, "Reserved GPT donation range cannot satisfy request 0x%x\n",
            create.x2);
        goto cleanup;
    }

    list = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (list == NULL)
        goto cleanup;

    val_memset(list, 0, PAGE_SIZE);
    /* RmiAddrRangeDesc: count in low bits and the aligned PA above them. */
    list[0] = base | count;
    donate = val_smc_call(RMI_OP_MEM_DONATE, handle, (uint64_t)list, 1U,
                          flags, 0, 0, 0, 0, 0, 0);
    if ((RMI_STATUS(donate.x0) != RMI_INCOMPLETE) ||
        (VAL_HOST_SRO_MEM_REQ(donate.x0) != RMI_OP_MEM_REQ_NONE) ||
        (donate.x1 != count))
    {
        LOG(ERROR, "Invalid GPT memory was not donated to the SRO, ret=0x%x\n",
            donate.x0);
        goto cleanup;
    }

    for (i = 0; i < VAL_HOST_SRO_MAX_ITERATIONS; i++)
    {
        cmd_ret = val_host_rmi_op_continue_smc(handle, RMI_CONTINUE_KEEP_GOING);
        status = RMI_STATUS(cmd_ret.x0);
        if (status == RMI_BUSY)
            continue;
        if ((status == RMI_INCOMPLETE) &&
            (VAL_HOST_SRO_MEM_REQ(cmd_ret.x0) == RMI_OP_MEM_REQ_NONE))
            continue;
        if (status == RMI_INCOMPLETE)
        {
            LOG(ERROR, "GPT creation requested an additional memory transfer\n");
            goto cleanup;
        }

        active = false;
        *result = cmd_ret.x0;
        ret = VAL_SUCCESS;
        if (status == RMI_SUCCESS)
        {
            if (val_host_rmi_gpt_l1_destroy(addr) != RMI_SUCCESS)
            {
                LOG(ERROR, "Unexpectedly created L1GPT could not be destroyed\n");
                ret = VAL_ERROR;
            }
        }
        break;
    }

cleanup:
    if (active)
    {
        if (VAL_EXTRACT_BITS(create.x0, 10, 10) == RMI_OP_CAN_CANCEL)
        {
            cancel = val_host_rmi_op_cancel_smc(handle);
            if (RMI_STATUS(cancel.x0) == RMI_INCOMPLETE)
            {
                cleanup_ret = val_host_sro_complete_with_handle(&cancel, handle);
                if (RMI_STATUS(cleanup_ret) != RMI_SUCCESS)
                    LOG(ERROR, "GPT donation SRO cancellation failed, ret=0x%x\n",
                        cleanup_ret);
            }
            else
            {
                LOG(ERROR, "GPT donation SRO could not be cancelled, ret=0x%x\n",
                    cancel.x0);
            }
        }
        else
        {
            cleanup_ret = val_host_sro_complete_with_handle(&create, handle);
            if (RMI_STATUS(cleanup_ret) == RMI_SUCCESS)
            {
                if (val_host_rmi_gpt_l1_destroy(addr) != RMI_SUCCESS)
                    LOG(ERROR, "GPT donation SRO cleanup could not destroy L1GPT\n");
            }
        }
    }
    if (list != NULL)
        val_host_mem_free(list);

    return ret;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    args->addr = c_args.addr_valid;

    switch (label)
    {
        case ADDR_BOUND:
            args->addr = addr_bound_prep_sequence(c_args.l0gpt_size);
            break;

        case ADDR_UNALIGNED:
            args->addr = g_unaligned_prep_sequence(c_args.addr_valid);
            break;

        case ENTRY_STATE:
            args->addr = entry_state_prep_sequence(c_args.addr_valid);
            if (args->addr == VAL_TEST_PREP_SEQ_FAILED)
                return VAL_ERROR;
            break;

        case UNFOLD_DENIED:
            args->addr = unfold_denied_prep_sequence(c_args.l0gpt_size);
            break;

        case GRAN_INVALID:
            if ((PLATFORM_GPT_L1_UNFOLDABLE_ADDR == 0U) ||
                (PLATFORM_GPT_L1_INVALID_DONATION_BASE == 0U) ||
                (PLATFORM_GPT_L1_INVALID_DONATION_SIZE == 0U))
                return VAL_SKIP_CHECK;

            if (entry_state_cleanup_sequence())
                return VAL_ERROR;

            args->addr = PLATFORM_GPT_L1_UNFOLDABLE_ADDR;
            if ((args->addr & (c_args.l0gpt_size - 1U)) != 0U)
            {
                LOG(ERROR, "GPT test address is not L0GPTSZ aligned\n");
                return VAL_ERROR;
            }
            break;

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

void cmd_gpt_l1_create_host(void)
{
    uint64_t ret, prep_ret, i;
    struct arguments args;

    if (valid_input_args_prep_sequence() == VAL_TEST_PREP_SEQ_FAILED) {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto exit;
    }

    for (i = 0; i < (sizeof(test_data) / sizeof(struct stimulus)); i++)
    {
        LOG(TEST, "Check %2d : %s; intent id : 0x%x \n",
              i + 1, test_data[i].msg, test_data[i].label);

        prep_ret = intent_to_seq(&test_data[i], &args);
        if (prep_ret == VAL_SKIP_CHECK)
        {
            LOG(TEST, "Check %d not exercised: no platform GPT test range\n",
                i + 1);
            continue;
        }
        if (prep_ret != VAL_SUCCESS)
        {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto free_addr;
        }

        if (test_data[i].label == GRAN_INVALID)
        {
            if (gran_invalid_donation_sequence(args.addr, &ret))
            {
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
                goto free_addr;
            }
        }
        else
        {
            ret = val_host_rmi_gpt_l1_create_smc(args.addr).x0;
        }

        if (ret != PACK_CODE(test_data[i].status, test_data[i].index))
        {
            LOG(ERROR, "Test Failure!The ABI call returned: %xExpected: %x\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto free_addr;
        }
    }

    if (entry_state_cleanup_sequence())
    {
        LOG(ERROR, "GPT L1 destroy cleanup failed\n");
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto free_addr;
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);
    ret = val_host_rmi_gpt_l1_create(c_args.addr_valid);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "GPT L1 create failed, ret=0x%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto free_addr;
    }
    c_args.entry_state_created = 1;

    ret = val_host_rmi_gpt_l1_destroy(c_args.addr_valid);
    if (ret != RMI_SUCCESS)
    {
        LOG(ERROR, "GPT L1 destroy observability failed, ret=0x%x\n", ret);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto free_addr;
    }
    c_args.entry_state_created = 0;

    val_set_status(RESULT_PASS(VAL_SUCCESS));

free_addr:
    entry_state_cleanup_sequence();
    if (c_args.alloc_valid != 0)
        val_host_mem_free((void *)c_args.alloc_valid);

exit:
    return;
}
