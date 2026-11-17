/*
 * Copyright (c) 2026, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_sro.h"
#include "command_common_host.h"
#include "rmi_op_continue_data.h"

#define RMI_OP_INVALID_HANDLE 0xFFFFFFFFFFFFFFFFULL

struct arguments {
    uint64_t handle;
    uint64_t flags;
    val_smc_param_ts sro_args;
    val_host_rec_params_ts *params;
    uint64_t rec;
    uint64_t rd;
    bool sro_started;
    bool use_cmd_ret;
};

static struct argument_store {
    struct arguments valid;
} c_args;

static void free_rec_params(struct arguments *args)
{
    if (args->params != NULL)
    {
        val_host_mem_free(args->params);
        args->params = NULL;
    }
}

static uint64_t validate_continue_output(val_smc_param_ts *cmd_ret)
{
    if ((RMI_STATUS(cmd_ret->x0) == RMI_INCOMPLETE) &&
        (VAL_HOST_SRO_MEM_REQ(cmd_ret->x0) == RMI_OP_MEM_REQ_DONATE))
        return VAL_SUCCESS;

    if (cmd_ret->x2 != 0U)
    {
        LOG(ERROR, "RMI_OP_CONTINUE returned unexpected X2: 0x%lx\n",
            cmd_ret->x2);
        return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t addr_range_desc(uint64_t addr, uint64_t count,
                                uint64_t state, uint64_t size)
{
    return size | (count << 2) | ADDR_ALIGN_DOWN(addr, PAGE_SIZE) |
           (state << 62);
}

static uint64_t prepare_donation_list(struct arguments *args, uint64_t *list_addr)
{
    uint64_t donate_req = args->sro_args.x2;
    uint64_t count = VAL_HOST_SRO_DONATE_REQ_COUNT(donate_req);
    uint64_t state = VAL_HOST_SRO_DONATE_REQ_STATE(donate_req);
    uint64_t size = VAL_HOST_SRO_DONATE_REQ_SIZE(donate_req);
    uint64_t donated_base;
    uint64_t donated_size;
    uint64_t *list;

    donated_size = count * PAGE_SIZE;
    if ((count == 0) || ((donated_size / PAGE_SIZE) != count))
        return VAL_ERROR;

    *list_addr = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    donated_base = (uint64_t)val_host_mem_alloc(PAGE_SIZE, donated_size);
    if ((*list_addr == 0) || (donated_base == 0))
    {
        if (*list_addr != 0)
            val_host_mem_free((void *)*list_addr);
        if (donated_base != 0)
            val_host_mem_free((void *)donated_base);
        *list_addr = 0;
        return VAL_ERROR;
    }

    if ((state == RMI_OP_MEM_DELEGATED) &&
        val_host_rmi_granule_range_delegate(donated_base,
                                            donated_base + donated_size))
    {
        val_host_mem_free((void *)*list_addr);
        *list_addr = 0;
        return VAL_ERROR;
    }

    list = (uint64_t *)*list_addr;
    val_memset(list, 0, PAGE_SIZE);
    list[0] = addr_range_desc(donated_base, count, state, size);

    return VAL_SUCCESS;
}

static uint64_t rec_create_mem_donate_prep_sequence(struct arguments *args,
                                                     bool mpidr_used)
{
    val_host_realm_ts realm;
    val_host_rec_params_ts rec_params;
    val_host_rec_params_ts *params;

    val_memset(&realm, 0, sizeof(realm));
    realm.s2sz = 40;
    realm.hash_algo = RMI_HASH_SHA_256;
    realm.s2_starting_level = 0;
    realm.num_s2_sl_rtts = 1;

    if (val_host_realm_create_common(&realm))
    {
        LOG(ERROR, "Realm create failed\n");
        return VAL_ERROR;
    }

    if (mpidr_used)
    {
        realm.rec_count = 1;
        val_memset(&rec_params, 0, sizeof(rec_params));
        if (val_host_rec_create_common(&realm, &rec_params))
        {
            LOG(ERROR, "Existing REC creation failed\n");
            return VAL_ERROR;
        }
    }

    args->rd = realm.rd;
    args->rec = g_delegated_prep_sequence();
    if (args->rec == VAL_TEST_PREP_SEQ_FAILED)
        return VAL_ERROR;

    args->params = val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (args->params == NULL)
        return VAL_ERROR;

    params = args->params;
    val_memset(params, 0, PAGE_SIZE);
    params->pc = 0;
    params->flags = RMI_RUNNABLE;
    params->mpidr = 0;
    params->num_aux = 0;

    args->sro_args = val_host_rmi_rec_create_smc(args->rd, args->rec,
                                                 (uint64_t)params);

    if (RMI_STATUS(args->sro_args.x0) != RMI_INCOMPLETE)
    {
        if (mpidr_used &&
            (RMI_STATUS(args->sro_args.x0) == RMI_ERROR_INPUT))
        {
            free_rec_params(args);
            return VAL_SKIP_CHECK;
        }

        if (!mpidr_used &&
            (RMI_STATUS(args->sro_args.x0) == RMI_SUCCESS))
        {
            val_host_update_granule_state(args->rd, GRANULE_REC,
                                          args->rec, 0, 0, 0);
            free_rec_params(args);
            return VAL_SKIP_CHECK;
        }

        LOG(ERROR, "REC_CREATE did not return RMI_INCOMPLETE, ret=0x%x\n",
            args->sro_args.x0);
        free_rec_params(args);
        return VAL_ERROR;
    }

    if (VAL_HOST_SRO_MEM_REQ(args->sro_args.x0) != RMI_OP_MEM_REQ_DONATE)
    {
        LOG(ERROR, "REC_CREATE SRO does not require donate, ret=0x%x\n",
            args->sro_args.x0);
        free_rec_params(args);
        return VAL_ERROR;
    }

    args->handle = args->sro_args.x1;
    args->flags = RMI_CONTINUE_KEEP_GOING;
    args->sro_started = true;

    return VAL_SUCCESS;
}

static void cleanup_rec_create_sro(struct arguments *args)
{
    uint64_t ret;

    if (!args->sro_started)
        return;

    if ((RMI_STATUS(args->sro_args.x0) != RMI_SUCCESS) &&
        (RMI_STATUS(args->sro_args.x0) != RMI_INCOMPLETE) &&
        (RMI_STATUS(args->sro_args.x0) != RMI_BUSY))
        args->sro_args = val_host_rmi_op_continue_smc(args->handle,
                                                      RMI_CONTINUE_KEEP_GOING);

    ret = val_host_sro_complete(&args->sro_args);
    if (ret == RMI_SUCCESS)
        val_host_update_granule_state(args->rd, GRANULE_REC,
                                      args->rec, 0, 0, 0);

    if ((RMI_STATUS(ret) != RMI_INCOMPLETE) &&
        (RMI_STATUS(ret) != RMI_BUSY))
        args->sro_started = false;
}

static uint64_t valid_continue_prep_sequence(struct arguments *args,
                                              bool mpidr_used)
{
    val_smc_param_ts donate_ret;
    uint64_t ret;
    uint64_t list_addr;

    ret = rec_create_mem_donate_prep_sequence(args, mpidr_used);
    if (ret)
        return ret;

    if (prepare_donation_list(args, &list_addr))
    {
        cleanup_rec_create_sro(args);
        free_rec_params(args);
        return VAL_ERROR;
    }

    donate_ret = val_host_rmi_op_mem_donate_smc(args->handle, list_addr, 1);
    val_host_mem_free((void *)list_addr);
    if ((RMI_STATUS(donate_ret.x0) != RMI_INCOMPLETE) ||
        (VAL_HOST_SRO_MEM_REQ(donate_ret.x0) != RMI_OP_MEM_REQ_NONE))
    {
        LOG(ERROR, "MEM_DONATE did not progress SRO, ret=0x%x\n",
            donate_ret.x0);
        args->sro_args = donate_ret;
        cleanup_rec_create_sro(args);
        free_rec_params(args);
        return VAL_ERROR;
    }

    args->sro_args = donate_ret;
    args->use_cmd_ret = true;
    return VAL_SUCCESS;
}

static uint64_t valid_input_args_prep_sequence(void)
{
    uint64_t ret;

    val_memset(&c_args, 0, sizeof(c_args));

    ret = valid_continue_prep_sequence(&c_args.valid, false);
    if (ret)
        return ret;

    return VAL_SUCCESS;
}

static uint64_t intent_to_seq(struct stimulus *test_data, struct arguments *args)
{
    enum test_intent label = test_data->label;

    val_memset(args, 0, sizeof(*args));
    args->flags = c_args.valid.flags;

    switch (label)
    {
        case HANDLE_INVALID:
            args->handle = RMI_OP_INVALID_HANDLE;
            break;

        case RESUMED_COMMAND_FAIL:
            return valid_continue_prep_sequence(args, true);

        default:
            LOG(ERROR, "Unknown intent label encountered\n");
            return VAL_ERROR;
    }

    return VAL_SUCCESS;
}

static uint64_t complete_sro(struct arguments *args, val_smc_param_ts cmd_ret)
{
    uint64_t ret;

    if (!args->sro_started)
        return VAL_SUCCESS;

    if (args->use_cmd_ret)
        args->sro_args = cmd_ret;

    ret = val_host_sro_complete(&args->sro_args);
    if (ret != RMI_SUCCESS)
        return ret;

    val_host_update_granule_state(args->rd, GRANULE_REC, args->rec, 0, 0, 0);
    args->sro_started = false;

    return VAL_SUCCESS;
}

void cmd_op_continue_host(void)
{
    uint64_t i, prep_ret, ret;
    val_smc_param_ts cmd_ret;
    struct arguments args;

    val_memset(&args, 0, sizeof(args));

    prep_ret = valid_input_args_prep_sequence();
    if (prep_ret == VAL_SKIP_CHECK)
    {
        LOG(TEST, "Skipping test: REC_CREATE completed without starting an SRO\n");
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto exit;
    }
    if (prep_ret)
    {
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
            LOG(TEST, "Skipping check: REC_CREATE reported mpidr_used before starting an SRO\n");
            continue;
        }
        if (prep_ret)
        {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto exit;
        }

        cmd_ret = val_host_rmi_op_continue_smc(args.handle, args.flags);

        if (test_data[i].label == RESUMED_COMMAND_FAIL)
        {
            if ((RMI_STATUS(cmd_ret.x0) != RMI_INCOMPLETE) ||
                (VAL_HOST_SRO_MEM_REQ(cmd_ret.x0) != RMI_OP_MEM_REQ_RECLAIM))
            {
                LOG(ERROR, "Failed REC_CREATE did not request reclaim, ret=0x%lx\n",
                    cmd_ret.x0);
                val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
                goto exit;
            }

            ret = val_host_sro_complete_with_handle(&cmd_ret, args.handle);
            args.sro_args = cmd_ret;
            if ((RMI_STATUS(ret) != RMI_INCOMPLETE) &&
                (RMI_STATUS(ret) != RMI_BUSY))
                args.sro_started = false;
        }
        else
        {
            ret = cmd_ret.x0;
        }

        if ((ret != PACK_CODE(test_data[i].status, test_data[i].index)) ||
            (cmd_ret.x2 != 0U)) {
            complete_sro(&args, args.sro_args);
            LOG(ERROR, "Test Failure!The ABI call returned: %lx Expected: %lx\n",
                ret, PACK_CODE(test_data[i].status, test_data[i].index));
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
            goto exit;
        }

        if (complete_sro(&args, args.sro_args)) {
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
            goto exit;
        }

        free_rec_params(&args);
    }

    LOG(TEST, "Check %2d : Positive Observability\n", ++i);
    cmd_ret = val_host_rmi_op_continue_smc(c_args.valid.handle,
                                           c_args.valid.flags);
    if (validate_continue_output(&cmd_ret))
    {
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto exit;
    }

    if (complete_sro(&c_args.valid, cmd_ret) ||
        (c_args.valid.sro_args.x0 != RMI_SUCCESS) ||
        (c_args.valid.sro_args.x2 != 0U))
    {
        LOG(ERROR, "RMI_OP_CONTINUE failed, ret=0x%lx, X2=0x%lx\n",
            c_args.valid.sro_args.x0, c_args.valid.sro_args.x2);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto exit;
    }

    cmd_ret = val_host_rmi_op_continue_smc(c_args.valid.handle,
                                           c_args.valid.flags);
    if ((cmd_ret.x0 != RMI_ERROR_INPUT) || (cmd_ret.x2 != 0U)) {
        LOG(ERROR, "Completed SRO handle remains valid, ret=0x%lx\n",
            cmd_ret.x0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto exit;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

exit:
    if (args.sro_started)
        complete_sro(&args, args.sro_args);
    free_rec_params(&args);

    if (c_args.valid.sro_started)
        complete_sro(&c_args.valid, c_args.valid.sro_args);
    free_rec_params(&c_args.valid);
    return;
}
